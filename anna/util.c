#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include "anna.h"


/* Construct a list of all the retriever packages */
di_package **
get_retriever_packages(di_packages *status)
{
  int ret_count = 0, ret_size = 4;
  di_package *package, **ret;
  di_slist_node *node;
  
  package = di_packages_get_package (status, "retriever", 0);

  if (!package)
    return NULL;

  ret = di_new0(di_package *, ret_size);

  for (node = package->depends.head; node; node = node->next)
  {
    di_package_dependency *d = node->data;

    if (d->type == di_package_dependency_type_reverse_provides)
      ret[ret_count++] = d->ptr;
    if (ret_count + 1 >= ret_size)
      break;
  }

  ret[ret_count] = NULL;
  return ret;
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
    char *retriever = NULL, *colon_p = NULL;

    debconf_get(debconf, ANNA_RETRIEVER);
    if (debconf->value != NULL)
        colon_p = strchr(debconf->value, ':');
    if (colon_p != NULL) {
        *colon_p = '\0';
        if (asprintf(&retriever, "%s/%s", RETRIEVER_DIR, debconf->value) == -1)
            retriever = NULL;
    }
    if (!retriever)
      di_log (DI_LOG_LEVEL_ERROR, "don't find retriever in %s, get %s", __PRETTY_FUNCTION__, debconf->value);
    return retriever;
}

/* Corresponds to the 'config' retriever command */
int
config_retriever(void)
{
    char *retriever, *command;
    int ret;

    retriever = get_retriever();
    if (asprintf(&command, "%s config", retriever) == -1)
        return 1;
    ret = di_exec_shell_log(command);
    free(command);
    return ret;
}

di_packages *
get_packages(di_packages_allocator *allocator)
{
    di_packages *packages;
    char *retriever, *command;
    int ret;

    retriever = get_retriever();
    if (asprintf(&command, "%s packages " DOWNLOAD_PACKAGES, retriever) == -1)
        return NULL;
    ret = di_exec_shell_log(command);
    free(command);
    if (ret != 0)
        return NULL;
    packages = di_system_packages_read_file(DOWNLOAD_PACKAGES, allocator);
    unlink(DOWNLOAD_PACKAGES);
    if (!packages)
      di_log(DI_LOG_LEVEL_ERROR, "can't find packages file");
    return packages;
}

/* This is not to be confused with di_pkg_is_installed which only checks
 * the package struct. This function checks if p is in the given list of
 * installed packages and compares versions. */
bool
is_installed(di_package *p, di_packages *status)
{
    di_package *q;
    di_package_version *pv, *qv;
    bool ret;

    /* If we don't understand the version number, we play safe
     * and assume we should install it */
    if (p->version == NULL || !(pv = di_package_version_parse(p)))
        return false;
    q = di_packages_get_package(status, p->package, 0);
    if (q == NULL || q->version == NULL || !(qv = di_package_version_parse(q)))
        return false;
    ret = (di_package_version_compare(pv, qv) <= 0);
    di_package_version_free(pv);
    di_package_version_free(qv);
    return ret;
}

size_t
package_to_choice(di_package *package, char *buf, size_t size)
{
  return snprintf(buf, size, "%s: %s", package->package, package->short_description);
}

char *
list_to_choices(di_package **packages)
{
    char buf[200], *ret;
    int count = 0;
    size_t ret_size = 1024, ret_used = 1, size;
    di_package *p;

    ret = di_malloc(1024);
    ret[0] = '\0';
    while ((p = packages[count])) {
        size = package_to_choice(p, buf, 200);
        if (ret_used + size + 2 > ret_size)
        {
            ret_size += 1024;
            ret = di_realloc(ret, ret_size);
        }
        strcat(ret, buf);
        ret_used += size + 2;
        count++;
        if (packages[count])
          strcat(ret, ", ");
    }
    if (ret_used)
        return ret;
    free(ret);
    return NULL;
}

/* Ask the chosen retriever to download a particular package to to dest. */
int
get_package (di_package *package, char *dest)
{
    int ret;
    char *retriever;
    char *command;

    retriever = get_retriever();
    if (asprintf(&command, "%s retrieve %s %s", retriever, package->filename, dest) == -1)
       return 1;
    ret = di_exec_shell_log(command);
    free(retriever);
    free(command);
    return ret;
}

/* Calls udpkg to unpack a package. */
#ifndef LIBDI_SYSTEM_DPKG
int
unpack_package (const char *pkgfile)
{
    char *command;
    int ret;

    if (asprintf(&command, "%s %s", DPKG_UNPACK_COMMAND, pkgfile) == -1)
        return 0;
    ret = !di_exec_shell_log(command);
    free(command);
    return ret;
}
#endif

/* check whether the md5sum of file matches sum.  if they don't,
 * return 0. */
int
md5sum(const char *sum, const char *file)
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
    if (asprintf(&command, "%s cleanup", retriever) != -1) {
        di_exec_shell_log(command);
        free(command);
    }
    free(retriever);
}

/* 
 * Simply return the XYZ in foo-modules-XYZ-udeb
 * Returns NULL if the match fails
 * FIXME: Should we cross-check against the package version?
 */
char *
udeb_kernel_version(di_package *p)
{
    char *name;
    char *t1, *t2;

    if (p->package == NULL)
	return NULL;
    name = p->package;
    if ((t1 = strstr(name, "-modules-")) == NULL)
        return NULL;
    t1 += sizeof("-modules-") - 1;
    if ((t2 = strstr(t1, "-udeb")) == NULL
	&& (t2 = strstr(t1, "-di")) == NULL)
        return NULL;
    if (t2[sizeof("-udeb") - 1] != '\0')
        return NULL;
    t2 = di_stradup(t1, t2 - t1);
    return t2;
}

int
package_array_compare(const void *v1, const void *v2)
{
    di_package *p1, *p2;

    p1 = *(di_package **)v1;
    p2 = *(di_package **)v2;
    return strcmp(p1->package, p2->package);
}

void
get_initial_package_list(di_packages *packages)
{
    di_package *p;
    FILE *fp;
    char buf[1024], *ptr;

    if ((fp = fopen(INCLUDE_FILE, "r")) == NULL)
        return;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (buf[0] == '#')
            continue;
        if ((ptr = strchr(buf, '\n')) != NULL)
            *ptr = '\0';
        p = di_packages_get_package(packages, ptr, 0);
        if (p)
          p->status_want = di_package_status_want_install;
    }
    fclose(fp);
}

void
drop_excludes(di_packages *packages)
{
    di_package *p;
    FILE *fp;
    char buf[1024], *ptr;

    if ((fp = fopen(EXCLUDE_FILE, "r")) == NULL)
        return;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (buf[0] == '#')
            continue;
        if ((ptr = strchr(buf, '\n')) != NULL)
            *ptr = '\0';
        p = di_packages_get_package(packages, ptr, 0);
        if (p)
          p->status_want = di_package_status_want_deinstall;
    }
    fclose(fp);
}

int
new_retrievers(di_package **retrievers_before, di_package **retrievers_after)
{
    int i, j;
    int match;

    for (i = 0; retrievers_before[i]; i++) {
	match = 0;
	for (j = 0; retrievers_after[j]; j++) {
	    if (strcmp(retrievers_before[i]->package,
                       retrievers_after[j]->package) == 0) {
		match = 1;
		break;
	    }
	}
	if (!match)
	    return 1;
    }
    return 0;
}

