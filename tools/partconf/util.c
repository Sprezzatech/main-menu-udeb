#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

char *
size_desc(long long bytes)
{
    static char ret[256];
    double kib, mib, gib;

    kib = bytes / 1024.0;
    mib = kib / 1024.0;
    gib = mib / 1024.0;
    if (gib > 10.0)
        sprintf(ret, "%.0f GiB", gib);
    else if (gib >= 1.0)
        sprintf(ret, "%.1f GiB", gib);
    else if (mib >= 100.0)
        sprintf(ret, "%.0f MiB", mib);
    else if (mib >= 1.0)
        sprintf(ret, "%.1f MiB", mib);
    else if (kib >= 100.0)
        sprintf(ret, "%.0f kiB", kib);
    else if (kib >= 1.0)
        sprintf(ret, "%.1f kiB", kib);
    else
        sprintf(ret, "%lld B", bytes);
    return ret;
}

void
modprobe(const char *mod)
{
    FILE *fp;
    char *cmd;
    char printk[1024] = "";

    if ((fp = fopen("/proc/sys/kernel/printk", "r")) != NULL) {
        fgets(printk, sizeof(printk), fp);
        fclose(fp);
    }
    if ((fp = fopen("/proc/sys/kernel/printk", "w")) != NULL) {
        fputs("0\n", fp);
        fclose(fp);
    }
    asprintf(&cmd, "modprobe %s 2>&1 >>/var/log/messages", mod);
    system(cmd);
    free(cmd);
    if ((fp = fopen("/proc/sys/kernel/printk", "w")) != NULL) {
        fputs(printk, fp);
        fclose(fp);
    }
}

/*
 * Check if something's already mounted on /target/mntpoint
 */
int
check_proc_mounts(const char *mntpoint)
{
    FILE *fp;
    char buf[1024], mnt[1024];
    char *tmp;

    if ((fp = fopen("/proc/mounts", "r")) == NULL)
        return 0;
    asprintf(&tmp, "/target%s", mntpoint);
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        sscanf(buf, "%*s %s", mnt);
        if (strcmp(tmp, mnt) == 0) {
            free(tmp);
            fclose(fp);
            return 1;
        }
    }
    free(tmp);
    fclose(fp);
    return 0;
}

/*
 * Check if the given device is already activated as a swap
 */
int
check_proc_swaps(const char *dev)
{
    FILE *fp;
    char buf[1024];

    if ((fp = fopen("/proc/swaps", "r")) == NULL)
        return 0;
    fgets(buf, sizeof(buf), fp);
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, dev) == buf) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void
append_message(const char *fmt, ...)
{
    FILE *fp;
    va_list ap;

    if ((fp = fopen("/var/log/messages", "a")) == NULL)
        return;
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    fclose(fp);
    va_end(ap);
}

/*
 * Counts the number of occurrences of c in s
 */
int
strcount(const char *s, int c)
{
    const char *p;
    int ret = 0;

    p = s;
    while ((p = index(p, c)) != NULL) {
        ret++;
        p++;
    }
    return ret;
}
