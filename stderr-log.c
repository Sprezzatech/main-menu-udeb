/* stderr logging and display for main-menu. */

#include "stderr-log.h"
#include <cdebconf/debconfclient.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

/* The log file is opened anew for each error, because it may be removed in
 * between. Not efficient, but the idea is that stderr should not happen. */
void log_error (const char *s) {
	FILE *f;

	f = fopen(STDERR_LOG, "a");
	fprintf(f, s);
	fflush(f);
	fclose(f);
}

/* Fork a listener process, that listens to the stderr of the main-menu
 * and anything it runs. */
void intercept_stderr(void) {
	pid_t pid;
	int filedes[2];

	if (pipe(filedes) != 0) {
		perror("pipe");
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(1);
	}

	if (pid) {
		close(filedes[0]);
		if (dup2(filedes[1], 2) == -1) {
			perror("dup2");
			exit(1);
		}
		return; /* go on with what main-menu does */
	}
	else {
		char buf[1024];
		FILE *f = fdopen(filedes[0], "r");

		close(filedes[1]);
		while ((fgets(&buf, 1024, f))) {
			log_error(buf);
		}
		exit(0);
	}
}

void display_stderr_log(const char *package) {
	static struct debconfclient *debconf = NULL;
	FILE *f;
	
	if (! debconf)
		debconf = debconfclient_new();
	
	if ((f = fopen(STDERR_LOG, "r"))) {
		int size = 0;
		char *p, *ret = NULL;

		if (feof(f))
			return;

		do {
			ret = realloc(ret, size + 128 + 1);
			if (! fgets(ret + size, 128, f)) {
				if (size == 0) {
					free(ret);
					return; /* eof with empty string */
				}
				else {
					ret[size]='\0';
					break;
				}
			}
			size = strlen(ret);
		} while (size > 0 && ! feof(f));
		
		/* remove newlines, as they screw up the debconf
		 * protocol. Which might one day be fixed.. */
		while ((p = strchr(ret, '\n')))
			p[0]=' ';
	
	// FIXME: package is NULL
	//	debconf->command(debconf, "TITLE", "Error in ", package);
		debconf->command(debconf, "SUBST", "debian-installer/generic_error", "ERROR", ret, NULL);
		debconf->command(debconf, "INPUT", "high", "debian-installer/generic_error", NULL);
		debconf->command(debconf, "GO", NULL);

		fclose(f);
		unlink(STDERR_LOG);
	}
}
