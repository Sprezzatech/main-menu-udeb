/**
 * @file debconfclient.h
 * @brief debconf client support interface
 *
 * \defgroup debconf client object
 * @{
 */
#ifndef _DEBCONFCLIENT_H_
#define _DEBCONFCLIENT_H_

#include <stdio.h>

/**
 * @brief debconf client object
 */
struct debconfclient {
    /** internal use only */
	char *value;

	/* methods */
    /**
     * @brief sends a command to the confmodule
     * @param struct debconfclient *client - client object
     * @param const char *command, ... - null terminated command string
     * @return return code from confmodule
     */
	int (*command)(struct debconfclient *client, const char *cmd, ...);

    /**
     * @brief sends a command to the confmodule (printf style)
     * @param struct debconfclient *client - client object
     * @param const char *commandf, printf style command format
     * @param ... - command parameters
     * @return return code from confmodule
     */
	int (*commandf)(struct debconfclient *client, const char *cmd, ...);

    /**
     * @brief simple accessor method for the return value
     * @param struct debconfclient *client - client object
     * @return char * - return value
     */
	char *(*ret)(struct debconfclient *client);

    /* Added to the end so we don't have to change the SONAME */
        FILE *out;
};

/**
 * @brief creates a debconfclient object
 * @return struct debconfclient * - newly created debconfclient object
 */
struct debconfclient *debconfclient_new(void);

/**
 * @brief destroys the debconfclient object 
 * @param struct debconfclient *client - client object to destroy
 */
void debconfclient_delete(struct debconfclient *client);

#define DEBCONF_OLD_STDIN_FD    4
#define DEBCONF_OLD_STDOUT_FD   5

/**
 * @}
 */

/* Helper macros for the debconf commands */
#define debconf_input(_client, _priority, _question) \
    _client->commandf(_client, "INPUT %s %s", _priority, _question)

#define debconf_clear(_client) \
    _client->commandf(_client, "CLEAR")

#define debconf_version(_client, _version) \
    _client->commandf(_client, "VERSION %u", _version)

#define debconf_capb(_client, _capb...) \
    _client->command(_client, "CAPB", ##_capb)

#define debconf_title(_client, _title) \
    _client->commandf(_client, "TITLE %s", _title)

#define debconf_beginblock(_client) \
    _client->command(_client, "BEGINBLOCK")

#define debconf_endblock(_client) \
    _client->command(_client, "ENDBLOCK")

#define debconf_go(_client) \
    _client->command(_client, "GO")

#define debconf_get(_client, _question) \
    _client->commandf(_client, "GET %s", _question)

#define debconf_set(_client, _question, _value) \
    _client->commandf(_client, "SET %s %s", _question, _value)

#define debconf_reset(_client, _question) \
    _client->commandf(_client, "RESET %s", _question)

#define debconf_subst(_client, _question, _var, _value) \
    _client->commandf(_client, "SUBST %s %s %s", _question, _var, _value)

#define debconf_register(_client, _template, _question) \
    _client->commandf(_client, "REGISTER %s %s", _template, _question)

#define debconf_unregister(_client, _question) \
    _client->commandf(_client, "UNREGISTER %s", _question)

#define debconf_purge(_client) \
    _client->command(_client, "PURGE")

#define debconf_metaget(_client, _question, _field) \
    _client->commandf(_client, "METAGET %s %s", _question, _field)

#define debconf_fget(_client, _question, _flag) \
    _client->commandf(_client, "FGET %s %s", _question, _flag)

#define debconf_fset(_client, _question, _flag, _value) \
    _client->commandf(_client, "FSET %s %s %s", _question, _flag, _value)

#define debconf_exists(_client, _question) \
    _client->commandf(_client, "EXIST %s", _question)

#define debconf_stop(_client) \
    _client->command(_client, "STOP")

#define debconf_progress_start(_client, _min, _max, _title) \
    _client->commandf(_client, "PROGRESS START %d %d %s", _min, _max, _title)

#define debconf_progress_step(_client, _step, _info) \
    _client->commandf(_client, "PROGRESS STEP %d %s", _step, _info)

#define debconf_progress_stop(_client) \
    _client->command(_client, "PROGRESS STOP")

#define debconf_x_loadtemplate(_client, _template) \
    _client->commandf(_client, "X_LOADTEMPLATEFILE %s", _template)

#endif
