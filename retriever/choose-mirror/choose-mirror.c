/*
 * Mirror selection via debconf.
 */

#include <cdebconf/debconfclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mirrors.h"
#ifdef WITH_HTTP
#include "mirrors_http.h"
#endif
#ifdef WITH_FTP
#include "mirrors_ftp.h"
#endif
#if ! defined (WITH_HTTP) && ! defined (WITH_FTP)
#error Must compile with at least one of FTP and HTTP
#endif

struct debconfclient *debconf;
char *protocol = NULL;
char *country  = NULL;

/*
 * Returns a string on the form "DEBCONF_BASE/protocol/supplied".  The
 * calling function is responsible for freeing the string afterwards.
 */

char *add_protocol(char *string) {
	char *ret;

	assert (protocol != NULL); /* Fetched by choose_protocol */
	asprintf(&ret,DEBCONF_BASE "%s/%s",protocol,string);
	return ret;
}

/*
 * Generates a list, suitable to be passed into debconf, from a
 * NULL-terminated string array.
 */
char *debconf_list(char *list[]) {
	int len, i, size = 1;
	char *ret = 0;
	
	for (i = 0; list[i] != NULL; i++) {
		len = strlen(list[i]);
		size += len;
		ret = realloc(ret, size + 2);
		memcpy(ret + size - len - 1, list[i], len);
		if (list[i+1] != NULL) {
			ret[size++ - 1] = ',';
			ret[size++ - 1] = ' ';
		}
		ret[size -1] = '\0';
	}
	return ret;
}

/*
 * Returns the correct mirror list, depending on whether protocol is
 * set to http or ftp.  Do NOT free the structure - it is a pointer to
 * the static list in mirrors_protocol.h
 */

struct mirror_t *mirror_list(void) {

	assert (protocol != NULL);

#ifdef WITH_HTTP
	if (strcasecmp(protocol,"http") == 0) {
		return mirrors_http;
	}
#endif
#ifdef WITH_FTP
	if (strcasecmp(protocol,"ftp") == 0) {
		return mirrors_ftp;
	}
#endif
	return 0; // should never happen
}

/* Returns an array of hostnames of mirrors in the specified country. */
char **mirrors_in(char *country) {
        static char **ret;
	int i, j, num = 1;
	struct mirror_t *mirrors = mirror_list();

	ret = malloc(num * sizeof(char *));
	for (i = j = 0; mirrors[i].country != NULL; i++) {
		if (j == num-1) {
			num *= 2;
			ret = realloc(ret,num * sizeof(char*));
		}
		if (strcmp(mirrors[i].country, country) == 0) {
			ret[j++]=mirrors[i].site;
		}
	}
	ret[j]=NULL;
	return ret;
}

inline int has_mirror(char *country)
{
	char **mirrors;
	mirrors = mirrors_in(country);
	return  (mirrors[0] == NULL) ? 0 : 1;
}

/* Returns the root of the mirror, given the hostname. */
char *mirror_root(char *mirror) {
	int i;

	struct mirror_t *mirrors = mirror_list();
	
	for (i = 0; mirrors[i].site != NULL; i++)
		if (strcmp(mirrors[i].site, mirror) == 0)
			return mirrors[i].root;

	return NULL;
}

int choose_country(void) {


	if (country)
		free(country);
	country = NULL;

	/* Pick a default country from elsewhere, eg languagechooser,*/
	debconf_get(debconf, DEBCONF_BASE "country");

	if (strcmp(debconf->value, "question will be asked") == 0) {
		// Not set yet. Seed with a default value
		if ((debconf_get(debconf, "debian-installer/country") == 0) &&
		    (strcmp(debconf->value, "question will be asked") != 0) &&
		    (debconf->value != NULL) ) {
				country = strdup (debconf->value);
				debconf_set (debconf, DEBCONF_BASE "country", country);
		}
	} else 
		country = debconf->value;

	// Ensure 'country' set to something
	if (strlen(country) == 0)
		country = NULL;
	if (country == NULL)
		country = "US";

#ifdef WITH_HTTP
	if (strcasecmp(protocol,"http") == 0) {
		if (has_mirror(country)) {
			debconf_set(debconf, DEBCONF_BASE "http/countries", country);
		}
		debconf_input(debconf, "medium", DEBCONF_BASE "http/countries");
	}
#endif
#ifdef WITH_FTP
	if (strcasecmp(protocol,"ftp") == 0) {
		if (has_mirror(country)) {
			debconf_set(debconf, DEBCONF_BASE "ftp/countries", country);
		}
 		debconf_input(debconf, "medium", DEBCONF_BASE "ftp/countries");
	}
#endif

	if (debconf_go (debconf))
		return 30; // goback
	
	debconf_get (debconf,  (strcasecmp(protocol,"http") == 0 ) ? 
			DEBCONF_BASE "http/countries" : DEBCONF_BASE "ftp/countries");
	country = strdup(debconf->value);
	debconf_set (debconf, DEBCONF_BASE "country", country);
	return 0;
}

/** 
 * @brief   Choose which protocol to use.
 * @returns retcode from debconf asking the question
 */ 
int choose_protocol(void) {
	int ret = 0;  
#if defined (WITH_HTTP) && defined (WITH_FTP)
	/* Both are supported, so ask. */
	debconf_subst(debconf, DEBCONF_BASE "protocol", "protocols", "http, ftp");
	debconf_input(debconf, "medium", DEBCONF_BASE "protocol");
	ret = debconf_go(debconf);
	debconf_get(debconf, DEBCONF_BASE "protocol");
	protocol = strdup(debconf->value);
#else
#ifdef WITH_HTTP
	debconf_set(debconf, DEBCONF_BASE "protocol", "http");
	protocol = "http";
#endif
#ifdef WITH_FTP
	debconf_set(debconf, DEBCONF_BASE "protocol", "ftp");
	protocol = "ftp";
#endif
#endif /* WITH_HTTP && WITH_FTP */
	return ret;
}

/* Choose which distribution to install. */
int choose_distribution(void) {
	int ret = 0;

	ret = debconf_input(debconf, "high", DEBCONF_BASE "distribution");
	ret = debconf_go (debconf);
	return ret;
}


int manual_entry;

int choose_mirror(void) {
	char *list;
	int ret = 0;

	debconf_get(debconf, DEBCONF_BASE "country");
	manual_entry = ! strcmp(debconf->value, "enter information manually");
	if (! manual_entry) {
                /* Prompt for mirror in selected country. */

		list=debconf_list(mirrors_in(country));
		debconf_subst(debconf, add_protocol("mirror"), "mirrors", list);
		free(list);
		
		ret = debconf_input(debconf, "medium", add_protocol("mirror"));
		
	}
	else {
		/* Manual entry. */
		ret = debconf_input(debconf, "critical", add_protocol("hostname"));
		if (ret == 0) {
			ret = debconf_input(debconf, "critical", add_protocol("directory"));
		}
	}
	/* Always ask about a proxy. */
	if (!ret) {
		ret = debconf_input(debconf,"high", add_protocol("proxy"));
	}

	ret = debconf_go (debconf);

	return ret;
}

int validate_mirror(void) {
	char *mirror;

	assert (protocol != NULL);

	if (! manual_entry) {
		/*
		 * Copy information about the selected
		 * mirror into mirror/{protocol}/{hostname,directory},
		 * which is the standard location other
		 * tools can look at.
		 */
		debconf_get(debconf, add_protocol("mirror"));
		mirror=strdup(debconf->value);
		debconf_set(debconf, add_protocol("hostname"), mirror);
		debconf_set(debconf, add_protocol("directory"),
		            mirror_root(mirror));
		free(mirror);
		return 0;
	} else {
		int not_ok = 0; /* Is 0 if everything is ok, 1 else, aka retval */
		/* Manual entry - check that the mirror is somewhat valid */
		debconf_get(debconf,  add_protocol("hostname"));
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			debconf_fset(debconf, add_protocol("hostname"), "seen", "false");
			not_ok = 1;
		}
		debconf_get(debconf,  add_protocol("directory"));	
		if (debconf->value == NULL || strcmp(debconf->value,"") == 0) {
			debconf_fset(debconf, add_protocol("directory"), "seen", "false");
			not_ok = 1;
		}
		return not_ok;
	}
	return 0;
}

int main (int argc, char **argv) {
	/* Use a state machine with a function to run in each state */
	int state = 0;
	int ret;
	int (*states[])() = {
		choose_protocol,
		choose_country,
		choose_mirror,
		validate_mirror,
                choose_distribution,
		NULL,
	};

	debconf = debconfclient_new();
	debconf_capb(debconf, "backup");
	debconf_version(debconf, 2);

	/*
	 * It's a pretty brain-dead state machine though. It advances
	 * forward and back by one state always. Enough for our purposes.
	 */
	while (state >= 0 && states[state]) {
		ret = states[state]();
			
		if (!ret) 
			state++;
		else
			state = (state == 4) ? 2 : (state-1); // Ugh. Fix.
	}
	if (state >= 0)
		exit(0);
	else
		exit(10); /* backed all the way out */
}
