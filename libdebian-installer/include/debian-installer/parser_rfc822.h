/*
 * parser_rfc822.h
 *
 * Copyright (C) 2003 Bastian Blank <waldi@debian.org>
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
 * $Id: parser_rfc822.h,v 1.1 2003/08/29 12:37:33 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__PARSER_RFC822_H
#define DEBIAN_INSTALLER__PARSER_RFC822_H

#include <debian-installer/hash.h>
#include <debian-installer/parser.h>

#include <stdio.h>

/**
 * @addtogroup di_parser_rfc822
 * @{
 */

int di_parser_rfc822_read (char *begin, size_t size, di_parser_info *fieldinfo, di_parser_read_entry_new entry_new, di_parser_read_entry_finish entry_finish, void *user_data);
int di_parser_rfc822_read_file (const char *file, di_parser_info *fieldinfo, di_parser_read_entry_new entry_new, di_parser_read_entry_finish entry_finish, void *user_data);
int di_parser_rfc822_write_file (const char *file, di_parser_info *fieldinfo, di_parser_write_entry_next entry_next, void *user_data);
  
/** @} */
#endif
