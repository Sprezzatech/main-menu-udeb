#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <cdebconf/debconfclient.h>
#include <debian-installer.h>
#include "anna.h"


/*
 * Helper functions for choose_retriever
 */

static int
is_retriever(struct package_t *p)
{
    int i;

    for (i = 0; p->provides[i] != 0; i++)
        if (strcmp(p->provides[i]->name, "retriever") == 0)
            return 1;
    return 0;
}

/* Construct a list of all the retriever packages */
struct linkedlist_t *
get_retriever_packages(void)
{
    FILE *fp;
    struct package_t *p;
    struct linkedlist_t *list;
    struct list_node *node, *tmp;

    fp = fopen(STATUS_FILE, "r");
    list = di_pkg_parse(fp);
    fclose(fp);
    while (list->head != NULL && !is_retriever((struct package_t *)list->head->data)) {
        di_pkg_free((struct package_t *)list->head->data);
        tmp = list->head->next;
        free(list->head);
        list->head = tmp;
    }
    node = list->head;
    while (node != NULL && node->next != NULL)
    {
        while (1)
        {
            if (node->next == NULL)
                break;
            /* half_configured means we have tried to configure
             * it before, and that failed. */
            p = (struct package_t *)node->next->data;
            if (!is_retriever(p) || p->status == half_configured) {
                di_pkg_free(p);
                tmp = node->next->next;
                free(node->next);
                node->next = tmp;
            } else
                break;
        }
        node = node->next;
    }
    return list;
}

/* Given a list of packages, construct a debconf choices string */
char *
get_retriever_choices(struct linkedlist_t *list)
{
    int retstr_size = 1;
    char *ret_choices;
    struct list_node *node;
    struct package_t *p;

    ret_choices = malloc(1);
    ret_choices[0] = '\0';
    for (node = list->head; node != NULL; node = node->next) {
        p = (struct package_t *)node->data;
        retstr_size += strlen(p->package) + 2 + strlen(p->description) + 2;
        ret_choices = realloc(ret_choices, retstr_size);
        strcat(ret_choices, p->package);
        strcat(ret_choices, ": ");
        strcat(ret_choices, p->description);
        strcat(ret_choices, ", ");
    }
    if (retstr_size >= 3)
        ret_choices[retstr_size-3] = '\0';
    return ret_choices;
}

/* Try retrievers in a sane turn. Also, see doc/retriever.txt */
const char *
get_default_retriever(const char *choices)
{
    static const char * const retrievers[] = {
        "net-retriever",
        "cdrom-retriever",
        "floppy-retriever",
        NULL
    };
    int i;

    for (i = 0; retrievers[i] != NULL; i++) {
        if (strstr(choices, retrievers[i]) != NULL)
            return retrievers[i];
    }
    return NULL;
}


/*
 * Helper functions for choose_modules
 */

char *
get_retriever(void)
{
    static struct debconfclient *debconf = NULL;
    char *retriever = NULL, *colon_p = NULL;

    if (debconf == NULL)
        debconf = debconfclient_new();
    debconf->command(debconf, "GET", ANNA_RETRIEVER, NULL);
    if (debconf->value != NULL)
        colon_p = strchr(debconf->value, ':');
    if (colon_p != NULL) {
        *colon_p = '\0';
        asprintf(&retriever, "%s/%s", RETRIEVER_DIR, debconf->value);
    }
    return retriever;
}

/* First try the 'packages' command, then guess a location */
static int
try_get_Packages_file(const char *ext, const char *suite)
{
    char *retriever, *command;
    int ret;

    retriever = get_retriever();
    asprintf(&command, "%s packages " DOWNLOAD_DIR "/Packages%s %s %s",
            retriever, ext, ext[0] == '\0' ? "." : ext, suite);
    ret = system(command);
    free(command);
    if (ret != 0) {
        static struct debconfclient *debconf = NULL;
        char *dist, *file;

        if (debconf == NULL)
            debconf = debconfclient_new();
        debconf->command(debconf, "GET", "mirror/distribution", NULL);
        dist = debconf->value;
        asprintf(&file, "dists/%s/%s/debian-installer/binary-%s/Packages%s",
                dist, suite, ARCH, ext);
        asprintf(&command, "%s retrieve %s " DOWNLOAD_DIR "/Packages%s",
                retriever, file, ext);
        free(file);
        ret = system(command);
        free(command);
    }
    return !ret;
}

struct linkedlist_t *
get_packages(void)
{
    FILE *fp;
    int i = 0;
    struct linkedlist_t *pkglist = NULL, *tmplist;
    /* TODO: This is a hack. But are Release files really a suitable replacement? */
    static const char *suites[] = { "main", "local", NULL };

    for (i = 0; suites[i] != NULL; i++) {
        fp = NULL;
        if (try_get_Packages_file(".gz", suites[i])) {
            if (system("gunzip " DOWNLOAD_DIR "/Packages.gz") == 0)
                fp = fopen(DOWNLOAD_DIR "/Packages", "r");
        }
        if (fp == NULL && try_get_Packages_file("", suites[i]))
            fp = fopen(DOWNLOAD_DIR "/Packages", "r");
        if (fp == NULL)
            continue;
        tmplist = di_pkg_parse(fp);
        fclose(fp);
        unlink(DOWNLOAD_DIR "/Packages");
        if (tmplist != NULL) {
            if (pkglist == NULL)
                pkglist = tmplist;
            else if (pkglist->tail == NULL) {
                free(pkglist);
                pkglist = tmplist;
            } else {
                pkglist->tail->next = tmplist->head;
                pkglist->tail = tmplist->tail;
                free(tmplist);
            }
        }
    }
    return pkglist;
}

/* This is not to be confused with di_pkg_is_installed which only checks
 * the package struct. This function checks if p is in the given list of
 * installed packages and compares versions. */
int
is_installed(struct package_t *p, struct linkedlist_t *installed)
{
    struct package_t *q;
    struct version_t pv, qv;
    int ret;

    /* If we don't understand the version number, we play safe
     * and assume we should install it */
    if (p->version == NULL || !di_parse_version(&pv, p->version))
        return 0;
    q = di_pkg_find(installed, p->package);
    if (q == NULL || q->version == NULL || !di_parse_version(&qv, q->version))
        return 0;
    ret = (di_compare_version(&pv, &qv) <= 0);
    free((void *)pv.version);
    free((void *)qv.version);
    return ret;
}

char *
list_to_choices(struct linkedlist_t *list)
{
    struct list_node *node;
    struct package_t *p;
    char *choices;
    size_t choices_size = 1;

    choices = malloc(choices_size);
    choices[0] = '\0';
    for (node = list->head; node != NULL; node = node->next) {
        p = (struct package_t *)node->data;
        choices_size += strlen(p->package) + 2 + strlen(p->description) + 2;
        choices = realloc(choices, choices_size);
        strcat(choices, p->package);
        strcat(choices, ": ");
        strcat(choices, p->description);
        strcat(choices, ", ");
    }
    if (choices_size >= 3)
        choices[choices_size-3] = '\0';
    return choices;
}

/* Ask the chosen retriever to download a particular package to to dest. */
int
get_package (struct package_t *package, char *dest)
{
    int ret;
    char *retriever;
    char *command;

    retriever = get_retriever();
    asprintf(&command, "%s retrieve %s %s", retriever, package->filename, dest);
    ret = !system(command);
    free(retriever);
    free(command);
    return ret;
}

/* Calls udpkg to unpack a package. */
int
unpack_package (char *pkgfile)
{
    char *command;
    int ret;

    asprintf(&command, "%s %s", DPKG_UNPACK_COMMAND, pkgfile);
    ret = !system(command);
    free(command);
    return ret;
}

/* check whether the md5sum of file matches sum.  if they don't,
 * return 0. */
int
md5sum(char *sum, char *file)
{
	FILE *fp;
	char line[1024];

	/* Trivially true if the Packages file doesn't have md5sum lines */
	if (sum == NULL)
		return 1;
	snprintf(line, sizeof(line), "/usr/bin/md5sum %s", file);
	fp = popen(line, "r");
	if (fp == NULL)
		return 0;
	if (fgets(line, sizeof(line), fp) != NULL) {
		fclose(fp);
		if (strlen(line) < 32)
			return 0;
		line[32] = '\0';
		return !strcmp(line, sum);
	}
	fclose(fp);
	return 0;
}

/* Corresponds to the retriever command 'cleanup' */
void
cleanup(void)
{
    char *retriever;
    char *command;

    retriever = get_retriever();
    asprintf(&command, "%s cleanup", retriever);
    system(command);
    free(retriever);
    free(command);
}
