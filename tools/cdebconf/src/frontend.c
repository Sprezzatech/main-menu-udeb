#include "common.h"
#include "configuration.h"
#include "database.h"
#include "frontend.h"
#include "question.h"

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
		   will we. */
		if (qlast == q)
			return DC_OK;
		qlast->next = q;
		q->prev = qlast;
	}

	question_ref(q);

	return DC_OK;
}

static int frontend_go(struct frontend *obj)
{
	return 0;
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

static void frontend_set_title(struct frontend *f, const char *title)
{
	DELETE(f->title);
	f->title = STRDUP(title);
}

static int frontend_can_go_back(struct frontend *ui, struct question *q)
{
	return 0;
}

static int frontend_can_go_forward(struct frontend *ui, struct question *q)
{
	return 1;
}

static void frontend_progress_start(struct frontend *ui, int min, int max, const char *title)
{
    DELETE(ui->progress_title);
	ui->progress_title = STRDUP(title);
    ui->progress_min = min;
    ui->progress_max = max;
    ui->progress_cur = min;
}

static void frontend_progress_set(struct frontend *ui, int val)
{
    ui->progress_cur = val;
}

static void frontend_progress_step(struct frontend *ui, int step)
{
    ui->methods.progress_set(ui, ui->progress_cur + step);
}

static void frontend_progress_info(struct frontend *ui, const char *info)
{
}

static void frontend_progress_stop(struct frontend *ui)
{
}

struct frontend *frontend_new(struct configuration *cfg, struct template_db *tdb, struct question_db *qdb)
{
	struct frontend *obj = NULL;
	void *dlh;
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
    modpath = cfg->get(cfg, "global::module_path::frontend", 0);
    if (modpath == NULL)
        DIE("Frontend module path not defined (global::module_path::frontend)");

    if (modname == NULL)
        DIE("Frontend instance driver not defined (%s)", tmp);

    setenv("DEBIAN_FRONTEND",modname,1);
    q = qdb->methods.get(qdb, "debconf/frontend");
    if (q)
	question_setvalue(q, modname);
    question_deref(q);
    snprintf(tmp, sizeof(tmp), "%s/%s.so", modpath, modname);
	if ((dlh = dlopen(tmp, RTLD_NOW)) == NULL)
		DIE("Cannot load frontend module %s: %s", tmp, dlerror());

	if ((mod = (struct frontend_module *)dlsym(dlh, "debconf_frontend_module")) == NULL)
		DIE("Malformed frontend module %s", modname);
	
	obj = NEW(struct frontend);
	memset(obj, 0, sizeof(struct frontend));
	obj->handle = dlh;
	obj->config = cfg;
	obj->tdb = tdb;
	obj->qdb = qdb;
    snprintf(obj->configpath, sizeof(obj->configpath),
        "frontend::instance::%s", modname);

    memcpy(&obj->methods, mod, sizeof(struct frontend_module));

#define SETMETHOD(method) if (obj->methods.method == NULL) obj->methods.method = frontend_##method

	SETMETHOD(initialize);
	SETMETHOD(shutdown);
	SETMETHOD(query_capability);
	SETMETHOD(set_title);
	SETMETHOD(add);
	SETMETHOD(go);
	SETMETHOD(clear);
	SETMETHOD(can_go_back);
	SETMETHOD(can_go_forward);
    SETMETHOD(progress_start);
    SETMETHOD(progress_set);
    SETMETHOD(progress_step);
    SETMETHOD(progress_info);
    SETMETHOD(progress_stop);

#undef SETMETHOD

	if (obj->methods.initialize(obj, cfg) == 0)
	{
		frontend_delete(obj);
		return NULL;
	}

	obj->capability = obj->methods.query_capability(obj);
	INFO(INFO_VERBOSE, "Capability: 0x%08X\n", obj->capability);

	return obj;
}

void frontend_delete(struct frontend *obj)
{
	obj->methods.shutdown(obj);
	dlclose(obj->handle);
	DELETE(obj->questions);
	DELETE(obj->capb);
	DELETE(obj->title);
    DELETE(obj->progress_title);
	DELETE(obj);
}

