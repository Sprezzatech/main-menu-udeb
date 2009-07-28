#include "common.h"
#include "configuration.h"
#include "plugin.h"
#include "database.h"
#include "frontend.h"
#include "question.h"
#include "template.h"
#include "strutl.h"

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

static int frontend_add(struct frontend *obj, struct question *q)
{
	struct question *qlast;
	ASSERT(q != NULL);
	ASSERT(q->prev == NULL);
	ASSERT(q->next == NULL);

	qlast = obj->questions;
	if (qlast == NULL)
	{
		obj->questions = q;
	}
	else
	{
		while (qlast != q && qlast->next != NULL)
		{
			qlast = qlast->next;
		}
		/* Question asked twice. debconf ignores the second question and
		   so will we. */
		if (qlast == q)
			return DC_OK;
		qlast->next = q;
		q->prev = qlast;
	}

	question_ref(q);

	return DC_OK;
}

static int frontend_add_noninteractive(struct frontend *obj, struct question *q)
{
	struct question *qlast;
	ASSERT(q != NULL);
	ASSERT(q->prev == NULL);
	ASSERT(q->next == NULL);

	//INFO(INFO_DEBUG, "adding noninteractive question");

	qlast = obj->questions_noninteractive;
	if (qlast == NULL)
	{
		obj->questions_noninteractive = q;
	}
	else
	{
		while (qlast != q && qlast->next != NULL)
		{
			qlast = qlast->next;
		}
		qlast->next = q;
		q->prev = qlast;
	}

	question_ref(q);

	return DC_OK;
}

static int frontend_go(struct frontend *obj)
{
	return DC_OK;
}

static int frontend_go_noninteractive(struct frontend *obj)
{
	struct question *q = obj->questions_noninteractive;

	while (q != NULL) {
		char *type = q->template->type;

		//INFO(INFO_DEBUG, "frontend_go_noninteractive; type %s", type);

		/* This is a hack to make noninteractive selects be set to
		 * the first item in the select list if their value is not
		 * set, or is set to something not in the list. This is for
		 * consistency with debconf. */
		if (strcmp(type, "select") == 0) {
			int i, ok=0;
			char **choices=NULL;
			char *val = (char *) question_getvalue(q, "");
			int count = strgetargc(q_get_choices_vals(obj, q));
			if (count) {
				choices = malloc(sizeof(char *) * count);
				if (strchoicesplit(q_get_choices_vals(obj, q), choices, count) != count)
					return DC_NOTOK;
			
					for (i = 0; i < count; i++) {
					if (val && strcmp(val, choices[i]) == 0) {
						ok=1;
						break;
					}
				}
			}

			if (! ok) {
				if (count)
					question_setvalue(q, choices[0]);
				else
					question_setvalue(q, "");
			}

			if (choices)
				free(choices);
		}

		q = q->next;
	}

	return DC_OK;
}

static void frontend_clear(struct frontend *obj)
{
	struct question *q;
	
	while (obj->questions != NULL)
	{
		q = obj->questions;
		obj->questions = obj->questions->next;
		q->next = q->prev = NULL;
		question_deref(q);
	}

	while (obj->questions_noninteractive != NULL)
	{
		q = obj->questions_noninteractive;
		obj->questions_noninteractive = obj->questions_noninteractive->next;
		q->next = q->prev = NULL;
		question_deref(q);
	}
}

static int frontend_initialize(struct frontend *obj, struct configuration *cfg)
{
	return DC_OK;
}

static int frontend_shutdown(struct frontend *obj)
{
	return DC_OK;
}

static unsigned long frontend_query_capability(struct frontend *f)
{
	return 0;
}

static const char * frontend_lookup_directive(struct frontend *obj,
                                              const char *directive)
{
	/* Remove unhandled directives. */
	return "";
}

static void frontend_set_title(struct frontend *f, const char *title)
{
	DELETE(f->title);
	f->title = STRDUP(title);
}

static void frontend_info(struct frontend *f, struct question *info)
{
	question_deref(f->info);
	f->info = info;
	question_ref(info);
}

static bool frontend_can_go_back(struct frontend *ui, struct question *q)
{
	return false;
}

static bool frontend_can_go_forward(struct frontend *ui, struct question *q)
{
	return true;
}

static bool frontend_can_cancel_progress(struct frontend *ui)
{
	return false;
}

static bool frontend_can_align(struct frontend *ui, struct question *q)
{
	return false;
}

static void frontend_progress_start(struct frontend *ui, int min, int max, struct question *title)
{
	question_deref(ui->progress_title);
	ui->progress_title = title;
	question_ref(ui->progress_title);
	ui->progress_min = min;
	ui->progress_max = max;
	ui->progress_cur = min;
}

static int frontend_progress_set(struct frontend *ui, int val)
{
	ui->progress_cur = val;
	return DC_OK;
}

static int frontend_progress_step(struct frontend *ui, int step)
{
	return ui->methods.progress_set(ui, ui->progress_cur + step);
}

static int frontend_progress_info(struct frontend *ui, struct question *info)
{
	return DC_OK;
}

static void frontend_progress_stop(struct frontend *ui)
{
}

struct frontend *frontend_new(struct configuration *cfg, struct template_db *tdb, struct question_db *qdb)
{
	struct frontend *obj = NULL;
	void *dlh = NULL;
	struct frontend_module *mod;
	char tmp[256];
	const char *modpath, *modname;
	struct question *q;

	modname = getenv("DEBIAN_FRONTEND");
	if (modname == NULL)
		modname = cfg->get(cfg, "_cmdline::frontend", 0);
	if (modname == NULL)
	{
		modname = cfg->get(cfg, "global::default::frontend", 0);
		if (modname == NULL)
			DIE("No frontend instance defined");

		snprintf(tmp, sizeof(tmp), "frontend::instance::%s::driver",
			modname);
		modname = cfg->get(cfg, tmp, 0);
	}
	if (modname == NULL)
		DIE("Frontend instance driver not defined (%s)", tmp);

	setenv("DEBIAN_FRONTEND", modname, 1);
	obj = NEW(struct frontend);
	memset(obj, 0, sizeof(struct frontend));

	modpath = cfg->get(cfg, "global::module_path::frontend", 0);
	if (modpath == NULL)
		DIE("Frontend module path not defined (global::module_path::frontend)");

	if (strcmp(modname, "none") != 0 && strcmp(modname, "noninteractive") != 0)
	{
		q = qdb->methods.get(qdb, "debconf/frontend");
		if (q)
			question_setvalue(q, modname);
		question_deref(q);
		snprintf(tmp, sizeof(tmp), "%s/%s.so", modpath, modname);
		//Frontend switching works when dlopening with RTLD_LAZY
		//The real reason why it segfaultes with RTLD_NOW has yet to be found
		if ((dlh = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL)) == NULL)
			DIE("Cannot load frontend module %s: %s", tmp, dlerror());

		if ((mod = (struct frontend_module *)dlsym(dlh, "debconf_frontend_module")) == NULL)
			DIE("Malformed frontend module %s", modname);
	
		memcpy(&obj->methods, mod, sizeof(struct frontend_module));
	}
	obj->name = strdup(modname);
	obj->handle = dlh;
	obj->config = cfg;
	obj->tdb = tdb;
	obj->qdb = qdb;
	frontend_set_title(obj, "");
	snprintf(obj->configpath, sizeof(obj->configpath),
 		"frontend::instance::%s", modname);

	if (asprintf(&obj->plugin_path, "%s/%s", modpath, modname) == -1) {
		frontend_delete(obj);
		 return NULL;
	}


#define SETMETHOD(method) if (obj->methods.method == NULL) obj->methods.method = frontend_##method

	SETMETHOD(initialize);
	SETMETHOD(shutdown);
	SETMETHOD(query_capability);
	SETMETHOD(lookup_directive);
	SETMETHOD(set_title);
	SETMETHOD(info);
	SETMETHOD(add);
	SETMETHOD(go);
	SETMETHOD(clear);
	SETMETHOD(can_go_back);
	SETMETHOD(can_go_forward);
	SETMETHOD(can_cancel_progress);
	SETMETHOD(can_align);
	SETMETHOD(progress_start);
	SETMETHOD(progress_set);
	SETMETHOD(progress_step);
	SETMETHOD(progress_info);
	SETMETHOD(progress_stop);
	SETMETHOD(add_noninteractive);
	SETMETHOD(go_noninteractive);

#undef SETMETHOD

	if (obj->methods.initialize(obj, cfg) == 0)
	{
		frontend_delete(obj);
		return NULL;
	}

	obj->capability = obj->methods.query_capability(obj);
	INFO(INFO_VERBOSE, "Capability: 0x%08lX", obj->capability);

	return obj;
}

void frontend_delete(struct frontend *obj)
{
	obj->methods.shutdown(obj);
	if (obj->handle != NULL)
		dlclose(obj->handle);
	frontend_clear(obj);
	DELETE(obj->capb);
	DELETE(obj->title);
	question_deref(obj->info);
	question_deref(obj->progress_title);
	DELETE(obj->plugin_path);
	DELETE(obj);
}

