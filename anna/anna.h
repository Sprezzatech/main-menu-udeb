#ifndef ANNA_H_
#define ANNA_H_ 1

#include <debian-installer.h>

#define STATUS_FILE             "/var/lib/dpkg/status"
#define RETRIEVER_DIR           "/usr/lib/debian-installer/retriever"
#define DOWNLOAD_DIR            "/var/cache/anna"
#define DPKG_UNPACK_COMMAND     "udpkg --unpack"
#define ANNA_RETRIEVER          "anna/retriever"
#define ANNA_CHOOSE_MODULES     "anna/choose_modules"

struct linkedlist_t *get_retriever_packages(void);
char *get_retriever_choices(struct linkedlist_t *list);
const char *get_default_retriever(const char *choices);
char *get_retriever(void);
struct linkedlist_t *get_packages(void);
int is_installed(struct package_t *p, struct linkedlist_t *installed);
char *list_to_choices(struct linkedlist_t *list);
int get_package (struct package_t *package, char *dest);
int unpack_package (char *pkgfile);
int md5sum(char* sum, char *file);
void cleanup(void);

#endif /* ANNA_H_ */
