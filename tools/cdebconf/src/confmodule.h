/**
 *
 * @file confmodule.h
 * @brief Configuration module object definition
 *
 */
#ifndef _CONFMODULE_H_
#define _CONFMODULE_H_

#include "common.h"

struct configuration;
struct template_db;
struct question_db;
struct frontend;

struct confmodule {
	struct configuration *config;
	struct template_db *templates;
	struct question_db *questions;
	struct frontend *frontend;
	pid_t pid;
	int infd, outfd;
	int exitcode;
	char *owner;
	char **seen_questions;

	/* methods */
    /*
     * @brief runs a config script, connected to the confmodule
     * @param struct confmodule *mod - confmodule object
     * @param int argc - number of arguments to pass to config script
     * @param char **argv - argument array
     * @return int - pid of config script, -1 if error
     */
	int (*run)(struct confmodule *, int argc, char **argv);

    /**
     * @brief handles communication between a config script and the 
     * confmodule
     * @param struct confmodule *mod - confmodule object
     * @return int - DC_OK, DC_NOTOK
     */
	int (*communicate)(struct confmodule *mod);

    /**
     * @brief process one command
     * @param const char *cmd - command to execute
     * @param char *out - return buffer
     * @param size_t outsize - size of return buffer
     * @return int - DC_OK, DC_NOTOK
     */
    int (*process_command)(struct confmodule *mod, char *cmd, 
        char *out, size_t outsize);

    /**
     * @brief Shuts down the confmodule
     * @param struct confmodule *mod - confmodule object
     * @return int - exit code of the config script
     */
	int (*shutdown)(struct confmodule *mod);

    /**
     * @brief Stack for already seen questions, to help backing up
     * @param struct confmodule *mod - confmodule object
     * @param int action - push, pop or sync values
     * @return int - DC_OK, DC_NOTOK
     */
	int (*update_seen_questions)(struct confmodule *mod, int action);
};

/**
 * @brief creates a new confmodule object
 * @param struct configuration *config - configuration parameters
 * @param struct database *db - database object
 * @param struct frontend *frontend - frontend UI object
 * @return struct confmodule * - newly created confmodule
 */
struct confmodule *confmodule_new(struct configuration *,
	struct template_db *, struct question_db *, struct frontend *);

/*
 * @brief destroys a confmodule object
 * @param confmodule *mod - confmodule object to destroy
 */
void confmodule_delete(struct confmodule *mod);

#endif
