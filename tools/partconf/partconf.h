#ifndef PARTCONF_H_
#define PARTCONF_H_ 1

#include <parted/parted.h>
#include <stdarg.h>

#define FS_ID_SWAP      "82"
#define FS_ID_LINUX     "83"
#define FS_ID_LVM       "8E"

#define PART_SIZE_BYTES(dev,part)       ((long long)(part)->geom.length * (long long)(dev)->sector_size)

#define MAX_DISCS       64
#define MAX_PARTS       1024
#define MAX_FSES        64

/* What we want to do with a partition */
struct operation {
    char                *filesystem; /* 'swap' is special case */
    char                *mountpoint;
    int                  done;
};

/* Represents a partition */
struct partition {
    char                *path;
    char                *fstype;
    char                *fsid;
    long long            size;
    struct operation     op;
};

extern char     *size_desc(long long bytes);
extern void      modprobe(const char *mod);
extern int       check_proc_mounts(const char *mntpoint);
extern void      append_message(const char *fmt, ...);

#endif /* PARTCONF_H_ */
