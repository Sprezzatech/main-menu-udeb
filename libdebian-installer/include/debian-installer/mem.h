/*
 * mem.h
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
 * $Id: mem.h,v 1.3 2003/11/19 09:24:14 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__MEM_H
#define DEBIAN_INSTALLER__MEM_H

#include <debian-installer/types.h>

#include <stdio.h>

/**
 * @addtogroup di_mem
 * @{
 */

void *di_malloc (size_t n_bytes) __attribute__ ((malloc));
void *di_malloc0 (size_t n_bytes) __attribute__ ((malloc));
void *di_realloc (void *mem, size_t n_bytes) __attribute__ ((malloc));
void di_free (void *mem);

/**
 * @param struct_type returned type
 * @param n_structs number of returned structs
 */
#define di_new(struct_type, n_structs) \
  ((struct_type *) di_malloc (sizeof (struct_type) * (n_structs)))
/**
 * @param struct_type returned type
 * @param n_structs number of returned structs
 */
#define di_new0(struct_type, n_structs) \
  ((struct_type *) di_malloc0 (sizeof (struct_type) * (n_structs)))
/**
 * @param struct_type returned type
 * @param mem current memory pointer
 * @param n_structs number of returned structs
 */
#define di_renew(struct_type, mem, n_structs) \
  ((struct_type *) di_realloc ((mem), sizeof (struct_type) * (n_structs)))

/** @} */
#endif
