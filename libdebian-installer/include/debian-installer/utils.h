/*
 * utils.h
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *               2003 Bastian Blank <waldi@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: utils.h,v 1.2 2003/11/03 13:46:12 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__UTILS_H
#define DEBIAN_INSTALLER__UTILS_H

/**
 * @defgroup di_utils Utils
 * @{
 */

/**
 * Inits the lib
 * @param progname name of the called binary
 */
void di_init (const char *progname);

/**
 * Get the name of the called binary
 * @return name
 */
const char *di_progname_get (void);

/** @} */
#endif
