#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

struct template_l10n_fields
{
	char *language;
	char *defaultval;
	char *choices;
	char *description;
	char *extended_description;
	struct template_l10n_fields *next;
};

struct template
{
	char *tag;
	unsigned int ref;
	char *type;
	struct template_l10n_fields *fields;
	struct template *next;
	const char *(*lget)(struct template *, const char *l, const char *f);
	const char *(*get)(struct template *, const char *f);
	void (*lset)(struct template *, const char *l, const char *f, const char *v);
	void (*set)(struct template *, const char *f, const char *v);
	const char *(*next_lang)(struct template *, const char *l);
};

extern const char *template_fields_list[];

struct template *template_new(const char *tag);
void template_delete(struct template *t);
void template_ref(struct template *t);
void template_deref(struct template *t);
struct template *template_dup(struct template *t);

struct template *template_load(const char *filename);

#endif

/*
Local variables:
c-file-style: "linux"
End:
*/
