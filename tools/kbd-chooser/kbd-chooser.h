/* @file kbd-chooser.h
 * 
 * Copyright (C) 2002 Alastair McKinstry   <mckinstry@computer.org>  
 * Released under the GNU License; see file COPYING for details 
 * 
 * $Id: kbd-chooser.h,v 1.2 2003/01/19 12:23:31 mckinstry Exp $
 */

#ifndef KBD_CHOOSER_H
#define KBD_CHOOSER_H

#define DEFMAP "us" /* Default Map */
#define KEYMAPDIR "keymaps"
#define KEYMAPLISTDIR "/usr/share/console/lists"

#define LINESIZE 512
#define DEBUG 1

typedef enum { TRUE = 1, FALSE = 0, UNKNOWN = -1 } exists;

typedef struct keymap_s {
	char *langs;
	char *name;
	char *description;
	struct keymap_s *next;
} keymap_t;

typedef struct maplist_s {
	char *name;
	keymap_t *maps;
	struct maplist_s *next;
} maplist_t;

typedef struct kbd_s { 
	char *name;
	char *description;
	char *deflt;
	exists present;
	int fd;
	keymap_t *keymap_list;
	struct kbd_s *next;
} kbd_t;

/* Some of the following will be linked in
 * via *-kbd.c
 */
extern kbd_t *at_kbd_get (void);
extern kbd_t *usb_kbd_get (void);
extern kbd_t *mac_kbd_get (void);
extern kbd_t *sparc_kbd_get (void);
extern kbd_t *amiga_kbd_get (void);
extern kbd_t *serial_kbd_get (void);
extern kbd_t *atari_kbd_get (void);

#endif
