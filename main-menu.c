/*
 * Debian Installer main menu program.
 *
 * Copyright 2000  Joey Hess <joeyh@debian.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "main-menu.h"
#include "stderr-log.h"
#include <cdebconf/debconfclient.h>
#include <stdlib.h>
#include <search.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static struct linkedlist_t *packages;

#ifdef DODEBUG
static int do_system(const char *cmd) {
	DPRINTF("cmd is %s\n", cmd);
	return system(cmd);
}
#endif

/*
 * qsort comparison function (sort by menu item values, fall back to
 * lexical sort to resolve ties deterministically).
 */
int compare (const void *a, const void *b) {
	/* Sometimes, I wish I was writing perl. */
	int r=(*(struct package_t **)a)->installer_menu_item -
	      (*(struct package_t **)b)->installer_menu_item;
	if (r) return r;
	return strcmp((*(struct package_t **)b)->package,
		      (*(struct package_t **)a)->package);
}

/* Returns true if the given package could be the default menu item. */
int isdefault(struct package_t *p) {
	int check;

	check = check_script(p, "menutest");
	if (check == -1) {
		if (p->status == unpacked || p->status == half_configured) {
			return 1;
		}
		else {
			return 0;
		}
	}
	else {
		return check;
	}
}


/* Calls a control skript and passes its exit status
 * Returns :
 *  0  if selected script returns false or something goes wrong
 *  1  if selected script return true
 * -1 if selected script is not present 
 */
int check_script(struct package_t *p, char *scriptname) {
	char *script, *cmd;
	struct stat statbuf;
	int ret;

	if (asprintf(&script, DPKGDIR "info/%s.%s", p->package, scriptname) == -1) {
		return 0;
	}
	if (stat(script, &statbuf) == 0) {
		if (asprintf(&cmd, "exec %s >/dev/null 2>&1", script) == -1) {
			return 0;
		}
		ret = !SYSTEM(cmd);
		free(cmd);
	}
	else {
		ret = -1;
	}
	free(script);
	return ret;
}

/* Expects a topologically ordered linked list of packages. */
static struct package_t *
get_default_menu_item(struct linkedlist_t *list)
{
	struct package_t *p, *q;
	struct list_node *node;
	int i, cont;

	/* Traverse the list, return the first menu item that isn't installed */
	for (node = list->head; node != NULL; node = node->next) {
		p = (struct package_t *)node->data;
		if (!p->installer_menu_item || p->status == installed || !check_script(p, "isinstallable"))
			continue;
		/* If menutest says this item should be default, make it so */
		if (isdefault(p))
			return p;
		cont = 0;
                /* Check if a "parallel" package is installed
		 * (netcfg-{static,dhcp} and {lilo,grub}-installer are
		 * examples of parallel packages */
		for (i = 0; p->provides[i] != NULL; i++) {
			q = p->provides[i]->ptr;
			if (q != NULL && di_pkg_is_installed(q)) {
				cont = 1;
				break;
			}
		}
		if (!cont)
			return p;
	}
	/* Severely broken, there are no menu items in the sorted list */
	return NULL;
}

/* Displays the main menu via debconf and returns the selected menu item. */
struct package_t *show_main_menu(struct linkedlist_t *list) {
	static struct debconfclient *debconf = NULL;
	char *language = NULL;
	struct package_t **package_list, *p;
	struct linkedlist_t *olist;
	struct list_node *node;
        struct package_t *menudefault = NULL, *ret = NULL;
	struct language_description *langdesc;
	int i = 0, num = 0;
	char *s;
	char menutext[1024];

	if (! debconf)
		debconf = debconfclient_new();
	
	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (language)
		free(language);
        if (debconf->value)
          language = strdup(debconf->value);
	/* Make a flat list of the packages. */
	for (node = list->head; node; node = node->next) {
		p = (struct package_t *)node->data;
		if (p->installer_menu_item)
			num++;
	}
	package_list = malloc(num * sizeof(struct package_t *));
	for (node = list->head; node; node = node->next) {
		p = (struct package_t *)node->data;
		if (p->installer_menu_item)
			package_list[i++] = (struct package_t *)node->data;
	}
	
	/* Sort by menu number. */
	qsort(package_list, num, sizeof(struct package_t *), compare);
	
	/* Order menu so depended-upon packages come first. */
	/* The menu number is really only used to break ties. */
	olist = di_pkg_toposort_arr(package_list, num);

	/*
	 * Generate list of menu choices for debconf.
	 */
	s = menutext;
	for (node = olist->head; node != NULL; node = node->next) {
		int ok = 0;
		p = (struct package_t *)node->data;
		if (!p->installer_menu_item || !check_script(p, "isinstallable"))
			continue;
		if (language) {
			langdesc = p->localized_descriptions;
			while (langdesc) {
				if (strcmp(langdesc->language,language) == 0) {
					/* Use this description */
					strcpy(s,langdesc->description);
					s += strlen(langdesc->description);
					ok = 1;
					break;
				}
				langdesc = langdesc->next;
			}
		}
		if (ok == 0) {
			strcpy(s, p->description);
			s += strlen(p->description);
		}
		*s++ = ',';
		*s++ = ' ';
	}
	/* Trim trailing ", " */
	if (s > menutext)
		s = s - 2;
	*s = 0;
	s = menutext;
	menudefault = get_default_menu_item(olist);
	free(olist);

	/* Make debconf show the menu and get the user's choice. */
        debconf->command(debconf, "TITLE", "Debian Installer Main Menu", NULL);
	debconf->command(debconf, "CAPB", NULL);
	debconf->command(debconf, "FSET", MAIN_MENU, "seen", "false", NULL);
	debconf->command(debconf, "SUBST", MAIN_MENU, "MENU", menutext, NULL);
	if (menudefault)
		debconf->command(debconf, "SET", MAIN_MENU, menudefault->description, NULL);
	debconf->command(debconf, "INPUT", "medium", MAIN_MENU, NULL);
	debconf->command(debconf, "GO", NULL);
	debconf->command(debconf, "GET", MAIN_MENU, NULL);
	s=debconf->value;
	
	/* Figure out which menu item was selected. */
	for (i = 0; i < num; i++) {
		p = package_list[i];
		if (strcmp(p->description, s) == 0) {
			ret = p;
			break;
		} else if (language) {
			for (langdesc = p->localized_descriptions; langdesc; langdesc = langdesc->next)
				if (strcmp(langdesc->language,language) == 0 &&
				    strcmp(langdesc->description,s) == 0) {
					ret = p;
					break;
				}
		}
	}
	free(language);
	free(package_list);
	return ret;
}

static void check_special(struct package_t *p);

/*
 * Satisfy the dependencies of a virtual package. Its dependencies
 * that actually provide the package are presented in a debconf select
 * question for the user to pick and choose. Other dependencies are
 * just fed recursively through di_config_package.
 */
static int satisfy_virtual(struct package_t *p) {
	struct debconfclient *debconf = NULL;
	struct package_t *dep, *defpkg = NULL;
	int i;
	char *choices, *tmp;
	size_t c_size = 1;
	int is_menu_item = 0;

        if (asprintf(&choices, "") == -1) {
		return 0;
	}
	/* Compile a list of providing package. The default choice will be the
	 * package with highest priority. If we have ties, menu items are
	 * preferred. If we still have ties, the default choice is arbitrary */
	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (!di_pkg_provides(dep, p)) {
			/* Non-providing dependency */
			if (dep->status != installed && !di_config_package(dep, satisfy_virtual, check_special))
				return 0;
			continue;
		}
		if (dep->status == installed) {
			/* This means that a providing package is already
			 * configure. So we short-circuit. */
			choices[0] = '\0';
			break;
		}
		if (defpkg == NULL || dep->priority > defpkg->priority ||
				(dep->priority == defpkg->priority &&
				 dep->installer_menu_item))
			defpkg = dep;
		/* This only makes sense if one of the dependencies
		 * is a menu item */
		if (dep->installer_menu_item)
			is_menu_item = 1;
		if (dep == defpkg) {
			/* We want the default to be the first item */
			if (asprintf(&tmp, "%s, %s", dep->description, choices) == -1) {
				return 0;
			}
		} else {
			if (asprintf(&tmp, "%s%s, ", choices, dep->description) == -1) {
				return 0;
			}
		}
		free(choices);
		choices = tmp;
	}
	c_size = strlen(choices);
	if (c_size >= 2)
		choices[c_size-2] = '\0';
	if (choices[0] != '\0') {
		if (is_menu_item) {
			char *priority = "medium";
			/* Only let the user choose if one of them is a menu item */
			debconf = debconfclient_new();
			debconf->command(debconf, "FSET", MISSING_PROVIDE, "seen",
					"false", NULL);
			if (defpkg != NULL)
				debconf->command(debconf, "SET", MISSING_PROVIDE,
						defpkg->description, NULL);
			else
				/* TODO: How to figure out a default? */
				priority = "critical";
			debconf->command(debconf, "CAPB backup", NULL);
			debconf->command(debconf, "SUBST", MISSING_PROVIDE,
					"CHOICES", choices, NULL);
			debconf->command(debconf, "INPUT", priority, MISSING_PROVIDE,
					NULL);
			if (debconf->command(debconf, "GO", NULL) != 0)
				return 0;
			debconf->command(debconf, "CAPB", NULL);
			debconf->command(debconf, "GET", MISSING_PROVIDE, NULL);
		}
		/* Go through the dependencies again */
		for (i = 0; p->depends[i] != 0; i++) {
			if ((dep = p->depends[i]->ptr) == NULL)
				continue;
			if (!di_pkg_provides(dep, p))
				continue;
			if (!is_menu_item || strcmp(debconf->value, dep->description) == 0) {
				/* Ick. If we have a menu item it has to match the
				 * debconf choice, otherwise we configure all of
				 * the providing packages */
				if (!di_config_package(dep, satisfy_virtual, check_special))
					return 0;
				if (is_menu_item)
					break;
			}
		}
	}
	if (debconf)
		debconfclient_delete(debconf);
	free(choices);
	/* It doesn't make sense to configure virtual packages,
	 * since they are, well, virtual. */
	p->status = installed;
	return 1;
}

static void update_language (void) {
	struct debconfclient *debconf;

	debconf = debconfclient_new();
	debconf->command(debconf, "GET", "debian-installer/language", NULL);
	if (*debconf->value != 0)
		setenv("LANGUAGE", debconf->value, 1);
	debconfclient_delete(debconf);
}

static void
check_special(struct package_t *p)
{
	int i;

	/*
	 * A language selection package must provide the virtual
	 * package 'language-selected'.
	 * The LANGUAGE environment variable must be updated
	 */
	for (i = 0; p->provides[i] != NULL; i++)
		if (strcmp(p->provides[i]->name, "language-selected") == 0) {
			update_language();
			break;
		}
}

int do_menu_item(struct package_t *p) {
	char *configcommand;
	int ret = 0;

	di_logf("Menu item '%s' selected", p->package);

	if (p->status == installed) {
		/* The menu item is already configured, so reconfigure it. */
		if (asprintf(&configcommand, "exec " DPKG_CONFIGURE_COMMAND " --force-configure %s", p->package) == -1) {
			return 0;
		}
                ret = SYSTEM(configcommand);
		free(configcommand);
		check_special(p);
                if (ret) {
                    di_logf("Reconfiguring '%s' failed with error code %d",
			    p->package, ret);
                }
                ret = !ret;
	}
	else if (p->status == unpacked || p->status == half_configured) {
		ret = di_config_package(p, satisfy_virtual, check_special);
	}

	return ret;
}

static char *debconf_priorities[] =
  {
    "low",
    "medium",
    "high",
    "critical"
  };

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
static void lower_debconf_priority (void) {
	int pri;
	const char *template = "debconf/priority";
	struct debconfclient *debconf = debconfclient_new();
	debconf->command(debconf, "GET", template, NULL);
	if ( ! debconf->value )
		pri = 1;
	else
		for (pri = 0; pri < ARRAY_SIZE(debconf_priorities); ++pri) {
			if (0 == strcmp(debconf->value,
					debconf_priorities[pri]) )
				break;
		}
	--pri;
	if (0 > pri)
		pri = 0;
	di_logf("Lowering debconf priority limit from '%s' to '%s'",
		debconf->value ? debconf->value : "(null)",
		debconf_priorities[pri] ? debconf_priorities[pri] : "(null)")

	debconf->command(debconf, "SET", template,
			 debconf_priorities[pri], NULL);
	debconfclient_delete(debconf);
}

int main (int argc, char **argv) {
	struct package_t *p;

	/* This spawns a process that traps all stderr from the rest of
	 * main-menu and the programs it calls, storing it in STDERR_LOG. */
	intercept_stderr();
	
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);

	packages = di_status_read();
	while ((p=show_main_menu(packages))) {
		if (!do_menu_item(p)) {
			di_logf("Menu item '%s' failed.", p->package);
			/* Something went wrong.  Lower debconf
			   priority limit to try to give the user more
			   control over the situation. */
			lower_debconf_priority();
		}
		
		/* Check for pending stderr in the stderr log, and
		 * display it in a nice debconf dialog. */
		/* XXX Pass in a title */
		display_stderr_log(p->package);

		di_list_free(packages, di_pkg_free);
		packages = di_status_read();
	
	}
	
	return(0);
}
