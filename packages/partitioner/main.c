#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/utsname.h>
#include <debian-installer.h>
#include <cdebconf/debconfclient.h>
#include <parted/parted.h>

#define MAX_DISKS 128

#define FDISK_PATH "/usr/share/partitioner"

static struct debconfclient *debconf;

static PedExceptionOption my_exception_handler(PedException* ex) {
	if (ex->options | PED_EXCEPTION_IGNORE ||
	    ex->type < PED_EXCEPTION_ERROR)
		return PED_EXCEPTION_IGNORE;

	return PED_EXCEPTION_CANCEL;
}

static int get_all_disks(PedDevice *discs[], int max_disks) {
	FILE *fp;
	char buf[1024];
	int disk_count = 0;

	fp = popen("list-devices disk", "r");
	if (fp == NULL) {
		di_log(DI_LOG_LEVEL_ERROR, "Failed to list disks");
		return(0);
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		PedDevice *dev;
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = '\0';
		if (disk_count >= max_disks) {
			di_log(DI_LOG_LEVEL_INFO, "More than %d discs", max_disks);
			break;
		}
		if ((dev = ped_device_get(buf)) && !dev->read_only)
			discs[disk_count++] = dev;
	}
	fclose(fp);
	return(disk_count);
}

static char *build_choice(PedDevice *dev) {
	char *string = NULL;
	int i;

	asprintf(&string, "%s (%s/ %.0fMiB)",
		dev->path, dev->model,
		dev->length * 1.0 * dev->sector_size / (1024*1024));

	/* remove ",", this will confuse debconf */
	for(i=0; string[i]; i++) {
		if(string[i] == ',')
			string[i] = ';';
	}

	return(string);
}

static char *extract_choice(const char *choice) {
	return strndup(choice, strcspn(choice, " "));
}

static char *execute_fdisk(void) {
	char *fdiskcmd = NULL;

	/*
	 *
	 * three-way fdisk running
	 *
	 */

	/* try to run arch fdisk command */
	asprintf(&fdiskcmd, "%s/%s.sh", FDISK_PATH, ARCH_TEXT);
	if(access(fdiskcmd, R_OK) == 0) {
		di_log(DI_LOG_LEVEL_INFO, "Using architecture-dependent fdisk configuration!");
		return(fdiskcmd);
	}

	/* fall back to common script, if exist */
	free(fdiskcmd);
	asprintf(&fdiskcmd, "%s/%s", FDISK_PATH, "common.sh");
	if(access(fdiskcmd, R_OK) == 0) {
		di_log(DI_LOG_LEVEL_INFO, "Using default fdisk configuration!");
		return(fdiskcmd);
	}

	/* fall back to >fdisk< */
	free(fdiskcmd);
	asprintf(&fdiskcmd, "%s", "/usr/sbin/fdisk");
	di_log(DI_LOG_LEVEL_INFO, "Fall back to default fdisk");
	return(fdiskcmd);
}

static int handler(pid_t pid, void *user_data) {
	if ((dup2(DEBCONF_OLD_STDIN_FD, 0) == -1) ||
	    (dup2(DEBCONF_OLD_STDOUT_FD, 1) == -1) ||
	    (dup2(DEBCONF_OLD_STDOUT_FD, 2) == -1))
		return 1;
	return 0;
}

int main(int argc, char *argv[]) {
	int disks_count, i;
	PedDevice *discs[MAX_DISKS];
	char *choices = NULL;

        di_system_init(basename(argv[0]));
	/* initialize debconf */
	debconf = debconfclient_new();

	/* hide libparted errors */
	ped_exception_set_handler(my_exception_handler);

	/* collect all discs from system, and build choices list */
	disks_count = get_all_disks(discs, MAX_DISKS);
	if(disks_count < 1) {
		debconf_set(debconf, "partitioner/nodiscs", "false");
		debconf_input(debconf, "high", "partitioner/nodiscs");
		debconf_go(debconf);
		return(EXIT_SUCCESS);
	}

	for(i=0; i<disks_count; i++) {
		char *tmp = NULL;

		tmp = build_choice(discs[i]);
		if(tmp == NULL)
			continue;

		if(choices == NULL) {
			asprintf(&choices, "%s", tmp);
		} else {
			asprintf(&choices, "%s, %s", choices, tmp);
		}

		free(tmp);
	}
	/* Christian Perrier : commented for removing hardcoded string
	   asprintf(&choices, "%s, %s", choices, "Finish"); */

	while(1) {
		char *cmd_script = NULL;
		char *cmd = NULL;
		char *disk = NULL;
		char *language = NULL;

		debconf_subst(debconf, "partitioner/disc", "DISCS", choices);
		debconf_set(debconf, "partitioner/disc", "false");
		debconf_input(debconf, "critical", "partitioner/disc");
		debconf_go(debconf);
		debconf_get(debconf, "partitioner/disc");

		/* exit the endless loop */
		if(strstr(debconf->value, "Finish"))
			break;

		cmd_script = execute_fdisk();
		disk = extract_choice(debconf->value);

		/* Hack for translated fdisks (mostly cfdisk) to be  */
		/* translated (tsl comes from the util-linux package */
		/* Comment if this causes problems with terminals    */
		/* not properly handling characters in translations  */
		debconf_get(debconf, "debian-installer/language");
		language = extract_choice(debconf->value);
		setenv("LANGUAGE", language, 1);

		if (strcmp(disk,"false") != 0) {
		  asprintf(&cmd, "/bin/sh %s %s", cmd_script, disk);

			i = di_exec_shell_full (cmd, NULL, NULL, NULL, NULL, NULL, handler, NULL);
			if(i != 0) {
				debconf_subst(debconf, "partitioner/fdiskerr", "DISC", disk);
				debconf_set(debconf, "partitioner/fdiskerr", "false");
				debconf_input(debconf, "high", "partitioner/fdiskerr");
				debconf_go(debconf);
			}
		}
		free(cmd_script);
		free(disk);
		free(cmd);
	}

	return(EXIT_SUCCESS);
}
