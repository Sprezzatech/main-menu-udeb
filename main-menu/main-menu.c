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
#include <cdebconf/debconfclient.h>
#include <stdlib.h>
#include <search.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

static struct linkedlist_t *packages;

#ifdef DODEBUG
static int do_system(const char *cmd) {
	DPRINTF("cmd is %s\n", cmd);
	return system(cmd);
}
#endif

/*
 * qsort comparison function (sort by menu item values, fallback to lexical
 * sort to resolve ties deterministically).
 */
int compare (const void *a, const void *b) {
	/* Sometimes, I wish I was writing perl. */
	int r=(*(struct package_t **)a)->installer_menu_item -
	      (*(struct package_t **)b)->installer_menu_item;
	if (r) return r;
	return strcmp((*(struct package_t **)b)->package,
		      (*(struct package_t **)a)->package);
}

#if 0
// This is commented out becuase I'm not sure we want to do this
/*
 * Builds a linked list of packages, ordered by dependencies, so
 * depended-upon packages come first. Pass the package to start ordering
 * at. head points to the start of the ordered list of packages, and tail
 * points to the end of the list (generally pass in pointers to NULL, unless
 * successive calls to the function are needed to build up a larger list).
 */
void order(struct package_t *p, struct package_t **head, struct package_t **tail) {
	struct package_t *found;
	int i;
	
	if (p->processed)
		return;
	
	for (i=0; p->depends[i] != 0; i++) {
		if ((found = tree_find(p->depends[i])))
			order(found, head, tail);
	}
	
	if (*head)
		(*tail)->next = *tail = p;
	else
		*head = *tail = p;
	
	(*tail)->next = NULL;
		
	p->processed = 1;
}

/*
 * Call this function after calling order to clear the processed tags.
 * Otherwise, later calls to order won't work.
 */
void order_done(struct package_t *head) {
	struct package_t *p;
	
	for (p=head; p; p=p->next)
		p->processed = 0;
}
#endif

/* Returns true if the given package could be the default menu item. */
int isdefault(struct package_t *p) {
	char *menutest, *cmd;
	struct stat statbuf;
	int ret;

	asprintf(&menutest, DPKGDIR "info/%s.menutest", p->package);
	if (stat(menutest, &statbuf) == 0) {
		asprintf(&cmd, "%s >/dev/null 2>&1", menutest);
		ret = !SYSTEM(cmd);
		free(cmd);
	}
	else if (p->status == unpacked || p->status == half_configured) {
		ret = 1;
	}
	else {
		ret = 0;
	}
	free(menutest);
	return ret;
}

/* The visit function for the depth-first traversal */
static void
dfs(struct package_t *p, struct linkedlist_t *queue)
{
	struct package_t *q;
	struct list_node *node;
	int i;

	p->processed = 1;
	for (i = 0; p->depends[i] != NULL; i++) {
		q = p->depends[i]->ptr;
		if (q == NULL)
			continue;
		if (!q->processed)
			dfs(q, queue);
	}
	/* Note that since we consider the list a queue and append in the end
	 * we will actually get a "reversed" toposort, but that's what we want.
	 */
	node = (struct list_node *)malloc(sizeof(struct list_node));
	node->data = p;
	node->next = NULL;
	if (queue->tail == NULL)
		queue->head = queue->tail = node;
	else {
		queue->tail->next = node;
		queue->tail = node;
	}
}

static struct package_t *
get_default_menu_item(struct package_t **packages, const int pkg_count)
{
	struct package_t *p, *q;
	struct linkedlist_t list;
	struct list_node *node;
	int i, cont;

	/* Topological sort of the packages, packages with fulfilled
	 * dependencies will come first */
	list.head = list.tail = NULL;
	for (i = 0; i < pkg_count; i++)
		packages[i]->processed = 0;
	for (i = 0; i < pkg_count; i++)
		if (!packages[i]->processed)
			dfs(packages[i], &list);
	/* Traverse the list, return the first menu item that isn't installed */
	for (node = list.head; node != NULL; node = node->next) {
		p = (struct package_t *)node->data;
		if (!p->installer_menu_item || p->status == installed)
			continue;
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
	struct list_node *node;
        struct package_t *menudefault = NULL;
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
#if 0
//I'm not sure this is a good idea... Maybe it's better to highlight the
//default choice in some other way instead?
	for (i = 0; i < num ; i++) {
		if (package_list[i]->installer_menu_item) {
			order(package_list[i], &head, &tail);
		}
	}
	order_done(head);
	free(package_list);
#endif
	
	/*
	 * Generate list of menu choices for debconf. Also figure out which
	 * is the default.
	 */
	s = menutext;
	for (i = 0; i < num; i++) {
		int ok = 0;
		p = package_list[i];
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
	menudefault = get_default_menu_item(package_list, num);

	/* Make debconf show the menu and get the user's choice. */
        debconf->command(debconf, "TITLE", "Debian Installer Main Menu", NULL);
	debconf->command(debconf, "FSET", MAIN_MENU, "seen", "false", NULL);
	debconf->command(debconf, "SUBST", MAIN_MENU, "MENU", menutext, NULL);
	if (menudefault)
		debconf->command(debconf, "SET", MAIN_MENU, menudefault->description, NULL);
	debconf->command(debconf, "INPUT medium", MAIN_MENU, NULL);
	debconf->command(debconf, "GO", NULL);
	debconf->command(debconf, "GET", MAIN_MENU, NULL);
	s=debconf->value;
	
	/* Figure out which menu item was selected. */
	for (i = 0; i < num; i++) {
		p = package_list[i];
		if (strcmp(p->description, s) == 0)
			return p;
		else if (language) {
			for (langdesc = p->localized_descriptions; langdesc; langdesc = langdesc->next)
				if (strcmp(langdesc->language,language) == 0 &&
				    strcmp(langdesc->description,s) == 0) {
					return p;
				}
		}
	}
	return NULL;
}

static int config_package(struct package_t *);

/*
 * Satisfy the dependencies of a virtual package. Its dependencies that actually
 * provide the package are presented in a debconf select question for the user
 * to pick and choose. Other dependencies are just fed recursively through
 * config_package.
 */
static int satisfy_virtual(struct package_t *p) {
	struct debconfclient *debconf;
	struct package_t *dep, *defpkg = NULL;
	int i;
	char *choices, *defval;
	size_t c_size = 1;
	int is_menu_item = 0;

	choices = malloc(1);
	choices[0] = '\0';
	/* Compile a list of providing package. The default choice will be the
	 * package with highest priority. If we have ties, menu items are
	 * preferred. If we still have ties, the default choice is arbitrary */
	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (!di_pkg_provides(dep, p)) {
			/* Non-providing dependency */
			if (dep->status != installed && !config_package(dep))
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
		c_size += strlen(dep->description) + 2;
		choices = realloc(choices, c_size);
		strcat(choices, dep->description);
		strcat(choices, ", ");
	}
	if (c_size >= 3)
		choices[c_size-3] = '\0';
	if (choices[0] != '\0') {
		if (is_menu_item) {
			/* Only let the user choose if one of them is a menu item */
			if (defpkg != NULL)
				defval = defpkg->description;
			else
				defval = "";
			debconf = debconfclient_new();
			debconf->command(debconf, "SUBST", MISSING_PROVIDE,
					"CHOICES", choices, NULL);
			debconf->command(debconf, "SET", MISSING_PROVIDE,
					defval, NULL);
			debconf->command(debconf, "INPUT medium", MISSING_PROVIDE,
					NULL);
			debconf->command(debconf, "GO", NULL);
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
				if (!config_package(dep))
					return 0;
				if (is_menu_item)
					break;
			}
		}
	}
	free(choices);
	/* It doesn't make sense to configure virtual packages,
	 * since they are, well, virtual. */
	p->status = installed;
	return 1;
}

/*
 * Configure all dependencies, special case for virtual packages.
 * This is done depth-first.
 */
static int
config_package(struct package_t *p) {
	char *configcommand;
	int ret, i;
	struct package_t *dep;

	for (i = 0; p->depends[i] != 0; i++) {
		if ((dep = p->depends[i]->ptr) == NULL)
			continue;
		if (dep->status == installed)
			continue;
		if (di_pkg_is_virtual(dep)) {
			if (!satisfy_virtual(dep))
				return 0;
		} else {
			/* Recursively configure this package */
			if (!config_package(dep))
				return 0;
		}
	}
	asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " %s", p->package);
	ret = SYSTEM(configcommand);
	free(configcommand);
        p->status = (ret == 0) ? installed : half_configured;
	return !ret;
}

int do_menu_item(struct package_t *p) {
	char *configcommand;
	int ret;

	if (p->status == installed) {
		/* The menu item is already configured, so reconfigure it. */
		asprintf(&configcommand, DPKG_CONFIGURE_COMMAND " --force-configure %s", p->package);
		ret = SYSTEM(configcommand);
		free(configcommand);
		return !ret;
	}
	else if (p->status == unpacked || p->status == half_configured) {
		config_package(p);
	}

	return 1;
}

int main (int argc, char **argv) {
	struct package_t *p;
	
	/* Tell udpkg to shut up. */
	setenv("UDPKG_QUIET", "y", 1);
	
	packages = status_read();
	while ((p=show_main_menu(packages))) {
		do_menu_item(p);
		packages = status_read();
	}
	
	return(0);
}
