/* @file  sparc-kbd.c
 * @brief Report keyboards present on Sun
 *
 * Copyright (C) 2003,2004 Alastair McKinstry, <mckinstry@debian.org>
 * Released under the GPL
 *
 * $Id$
 */

#include "config.h"
#include <assert.h>
#include <debian-installer.h>
#include "xmalloc.h"
#include "kbd-chooser.h"


/**
 * @brief list of keyboards present
 */
kbd_t *sparc_kbd_get (kbd_t *keyboards, const char *subarch)
{
	kbd_t *k = xmalloc (sizeof(kbd_t));

	k->name = "sun"; // This must match the name "sun" in console-keymaps-sun
	k->deflt = NULL;
	k->data = NULL;
	k->present = UNKNOWN;
	k->next = keyboards;
	keyboards = k;
	
	/* In 2.6 series, we can detect keyboard via /proc/bus/input
	 *
	 * TODO:
	 * Its possible to read the keyboard type; use this to preselect
	 * keymap (edit the keymaps list)
	 */

	  // /proc must be mounted by this point
	  // assert (check_dir ("/proc") == 1); 
	
	  if (check_dir ("/proc/bus/input")) {
		int res;
		// this dir only present in 2.6
		res = grep ("/proc/bus/input/devices","Sun Type");
		if (res < 0) {
			di_info ("sparc-kbd: Could not open /proc/bus/input/devices");
			return keyboards;
		}
		k->present = (res == 0) ? TRUE : FALSE;
	}	

	return keyboards;
}
