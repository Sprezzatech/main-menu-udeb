#ifndef _COMMON_H_
#define _COMMON_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "debug.h"

#define DC_NOTOK	0
#define DC_OK		1
#define DC_NOTIMPL	2
#define DC_AUTHNEEDED	3

#define DC_GOBACK	30

#define DC_NO		0
#define DC_YES		1

#define INFO_ERROR	0
#define INFO_WARN	1
#define INFO_DEBUG	5
#define INFO_VERBOSE	20

#define DIE(fmt, args...) 					\
 	do {							\
		fprintf(stderr, fmt, ##args);			\
		fprintf(stderr, " at %s line %u\n", __FILE__, __LINE__); \
		exit(1);					\
	} while(0)

#ifndef NODEBUG
#define INFO(level, fmt, args...)					\
	debug_printf(level, fmt, ##args)
#define ASSERT(cond) if (!(cond)) DIE("Assertion failed: %s", #cond)
#else
#define INFO(level, fmt, args...)	/* nothing */
#define ASSERT(cond)
#endif

#define NEW(type) (type *)malloc(sizeof(type))
#define DELETE(x) do { free(x); x = 0; } while (0)
#define CHOMP(s) if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = 0 

/* Be careful with these macros; they evaluate the string arguments multiple
   times!
 */
#define STRDUP(s) ((s) == NULL ? NULL : strdup(s))
#define STRLEN(s) ((s) == NULL ? 0 : strlen(s))
#define STRCPY(d,s) strcpy(d,((s) == NULL ? "" : (s)))
#define DIM(ar) (sizeof(ar)/sizeof(ar[0]))

/* MIN and MAX are also defined from perl.h, called from perldb.c */
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif
