#include "confmodule.h"
#include "commands.h"
#include "frontend.h"
#include "database.h"
#include "question.h"
#include "strutl.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

static commands_t commands[] = {
    { "input",	    command_input },
    { "clear",	    command_clear },
    { "version",	command_version },
    { "capb",	    command_capb },
    { "title",	    command_title },
    { "beginblock",	command_beginblock },
    { "endblock", 	command_endblock },
    { "go",		    command_go },
    { "get",	    command_get },
    { "set",	    command_set },
    { "reset",	    command_reset },
    { "subst",	    command_subst },
    { "register",	command_register },
    { "unregister",	command_unregister },
    { "purge",	    command_purge },
    { "metaget",	command_metaget },
    { "fget",	    command_fget },
    { "fset",	    command_fset },
    { "exist",	    command_exist },
    { "stop",	    command_stop },
    { "progress",   command_progress },
    { "x_loadtemplatefile", command_x_loadtemplatefile },
    { 0, 0 }
};

/* private functions */
/*
 * @brief helper function to process incoming commands
 * @param struct confmodule *mod - confmodule object
 * @param char *in - input command
 * @param char *out - reply buffer
 * @param size_t outsize - reply buffer length
 * @return int - DC_OK, DC_NOTOK, DC_NOTIMPL
 */
static int _confmodule_process(struct confmodule *mod, char *in, char *out, size_t outsize)
{
	int i = 0, argc;
	char *argv[10];

	out[0] = 0;
	if (*in == '#') return 1;

	memset(argv, 0, sizeof(char *) * DIM(argv));
	argc = strcmdsplit(in, argv, DIM(argv) - 1);

	for (; commands[i].command != 0; i++)
	{
		if (strcasecmp(argv[0], commands[i].command) == 0) {
			return (*commands[i].handler)(mod, argc - 1, argv, 
							out, outsize);
		}
	}
	return DC_NOTOK;
}

/* public functions */
static int confmodule_communicate(struct confmodule *mod)
{
    char buf[1023];
    char *in;
    size_t insize = 1024;
  	char *out;
    size_t outsize = 1024;
	char *inp;
	int ret = 0;

    in = malloc(insize);
    if (!in)
        return DC_NOTOK;
    memset(in, 0, insize);

    out = malloc(outsize);
    if (!out)
        return DC_NOTOK;
    memset(out, 0, outsize);

	while (1) {
        buf[0] = 0;
        in[0] = 0;
        while (strchr(buf, '\n') == NULL) {
            ret = read(mod->infd, buf, sizeof(buf));
            if (ret <= 0)
                return DC_NOTOK;
            buf[ret] = 0;
            if (strlen(in) + ret + 1 > insize) {
                insize += sizeof(buf);
                in = realloc(in, insize);
            }
            strcat(in, buf);
        }
        
        inp = strstrip(in);
        INFO(INFO_DEBUG, "--> %s\n", inp);
        ret = _confmodule_process(mod, inp, out, outsize);
        if (ret == DC_NOTIMPL) {
            fprintf(stderr, "E: Unimplemented function\n");
            continue;
        }
        /*		if (out[0] == 0) break; // STOP called*/
        INFO(INFO_DEBUG, "<-- %s\n", out);
		strcat(out, "\n");
		write(mod->outfd, out, strlen(out));
	}
	return ret;
}

static int confmodule_process_command(struct confmodule *mod, char *cmd, 
        char *out, size_t outsize)
{
    int ret;
    char *inp;

    inp = strstrip(cmd);

    INFO(INFO_DEBUG, "--> %s\n", inp);
    ret = _confmodule_process(mod, inp, out, outsize);
    if (ret == DC_NOTIMPL) {
        fprintf(stderr, "E: Unimplemented function\n");
        snprintf(out, outsize, "%u Not implemented", ret);
    }
    /*		if (out[0] == 0) break; // STOP called*/
    INFO(INFO_DEBUG, "<-- %s\n", out);

    return ret;
}

static int confmodule_shutdown(struct confmodule *mod)
{
    int status;

    mod->exitcode = 0;

    waitpid(mod->pid, &status, 0);

    if (WIFEXITED(status))
        mod->exitcode = WEXITSTATUS(status);

    return mod->exitcode;
}

static int confmodule_run(struct confmodule *mod, int argc, char **argv)
{
	int pid;
	int i;
	char **args;
	int toconfig[2], fromconfig[2]; /* 0=read, 1=write */
	pipe(toconfig);
	pipe(fromconfig);
	switch ((pid = fork()))
	{
	case -1:
		mod->frontend->methods.shutdown(mod->frontend);
		DIE("Cannot execute client config script");
		break;
	case 0:
		close(fromconfig[0]); close(toconfig[1]);
		if (toconfig[0] != 0) { /* if stdin is closed initially */
			dup2(toconfig[0], 0); close(toconfig[0]);
		}
		dup2(fromconfig[1], 1); close(fromconfig[1]);

		args = (char **)malloc(sizeof(char *) * argc-1);
		for (i = 1; i < argc; i++)
			args[i-1] = argv[i];
		args[argc-1] = NULL;
		if (execv(argv[1], args) != 0)
			perror("execv");
		/* should never reach here, otherwise execv failed :( */
		DIE("Cannot execute client config script");
	default:
		close(fromconfig[1]); close(toconfig[0]);
		mod->infd = fromconfig[0];
		mod->outfd = toconfig[1];
	}

	return pid;
}

static int confmodule_update_seen_questions(struct confmodule *mod, enum seen_action action)
{
	struct question *q;
	struct question *qlast = NULL;
	int i, narg;

	switch (action)
	{
	case STACK_SEEN_ADD:
		if (mod->seen_questions == NULL)
			narg = 0;
		else
			narg = sizeof(mod->seen_questions) / sizeof(char *);

		i = narg;
		for (q = mod->frontend->questions; q != NULL; q = q->next)
			narg++;
		if (narg == 0)
			return DC_OK;

		mod->seen_questions = (char **) realloc(mod->seen_questions, narg);
		for (q = mod->frontend->questions; q != NULL; q = q->next)
		{
			*(mod->seen_questions+i) = strdup(q->tag);
			i++;
		}
		break;
	case STACK_SEEN_REMOVE:
		if (mod->seen_questions == NULL)
			return DC_OK;

		narg = sizeof(mod->seen_questions) / sizeof(char *);
		for (q = mod->frontend->questions; q != NULL; q = q->next)
			qlast = q;

		for (q = qlast; q != NULL; q = q->prev)
		{
			if (strcmp(*(mod->seen_questions + narg - 1), q->tag) != 0)
				return DC_OK;
			DELETE(*(mod->seen_questions + narg - 1));
			narg --;
		}
		break;
	case STACK_SEEN_SAVE:
		if (mod->seen_questions == NULL)
			return DC_OK;

		narg = sizeof(mod->seen_questions) / sizeof(char *);
		for (i = 0; i < narg; i++)
		{
			q = mod->questions->methods.get(mod->questions, *(mod->seen_questions+i));
			if (q == NULL)
				return DC_NOTOK;
			q->flags |= DC_QFLAG_SEEN;
			DELETE(*(mod->seen_questions+i));
		}
		DELETE(mod->seen_questions);
		break;
	default:
		/* should never happen */
		DIE("Mismatch argument in confmodule_update_seen_questions");
	}

	return DC_OK;
}

struct confmodule *confmodule_new(struct configuration *config,
	struct template_db *templates, struct question_db *questions, 
    struct frontend *frontend)
{
	struct confmodule *mod = NEW(struct confmodule);

	mod->config = config;
	mod->templates = templates;
	mod->questions = questions;
	mod->frontend = frontend;
	mod->seen_questions = NULL;
	mod->run = confmodule_run;
	mod->communicate = confmodule_communicate;
	mod->process_command = confmodule_process_command;
	mod->shutdown = confmodule_shutdown;
	mod->update_seen_questions = confmodule_update_seen_questions;

	/* TODO: I wish we don't need gross hacks like this.... */
	setenv("DEBIAN_HAS_FRONTEND", "1", 1);

	return mod;
}

void confmodule_delete(struct confmodule *mod)
{
	DELETE(mod);
}

