/*
 * mem_chunk.h
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
 * $Id: mem_chunk.h,v 1.1 2003/11/19 09:24:14 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__MEM_CHUNK_H
#define DEBIAN_INSTALLER__MEM_CHUNK_H

#include <debian-installer/types.h>

typedef struct di_mem_chunk di_mem_chunk;

/**
 * @addtogroup di_mem_chunk
 * @{
 */

di_mem_chunk* di_mem_chunk_new (di_ksize_t atom_size, di_ksize_t area_size) __attribute__ ((nonnull));
void *di_mem_chunk_alloc (di_mem_chunk *mem_chunk);
void *di_mem_chunk_alloc0 (di_mem_chunk *mem_chunk);
void di_mem_chunk_destroy (di_mem_chunk *mem_chunk);

/** @} */
#endif
