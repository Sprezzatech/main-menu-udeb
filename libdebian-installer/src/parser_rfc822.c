/*
 * parser_rfc822.c
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
 * $Id: parser_rfc822.c,v 1.3 2003/09/24 11:49:52 waldi Exp $
 */

#include <config.h>

#include <debian-installer/parser_rfc822.h>

#include <debian-installer/macros.h>
#include <debian-installer/string.h>

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define READSIZE 16384

/**
 * Parse a rfc822 formated file
 *
 * @param begin begin of memory segment
 * @param size size of memory segment
 * @param info parser info
 * @param user_data user_data for parser functions
 *
 * @return number of parsed entries
 */
int di_parser_rfc822_read (char *begin, size_t size, di_parser_info *info, di_parser_read_entry_new entry_new, di_parser_read_entry_finish entry_finish, void *user_data)
{
  char *cur, *end;
  char *field_begin, *field_end, *field_modifier_begin, *field_modifier_end, *value_begin, *value_end;
#ifndef HAVE_MEMRCHR
  char *temp;
#endif
  int nr = 0;
  size_t readsize, field_size, field_modifier_size, value_size;
  const di_parser_fieldinfo *fip = NULL;
  di_rstring field_string, field_modifier_string, value_string;
  void *act = NULL;

  cur = begin;
  end = begin + size;

  while (cur < end)
  {
    nr++;

    if (entry_new)
      act = entry_new (user_data);
    else
      act = NULL;

    while (1)
    {
      field_begin = cur;
      readsize = end - field_begin < READSIZE ? end - field_begin : READSIZE;
      if (!readsize)
        break;
      field_modifier_end = field_end = memchr (cur, ':', readsize);
      if (!field_end)
      {
        di_warning ("parser_rfc822: Iek! Don't find end of field!");
        return -1;
      }
      field_size = field_end - field_begin;

#ifdef HAVE_MEMRCHR
      if ((field_modifier_begin = memrchr (field_begin, '-', field_end - field_begin)))
        field_modifier_begin++;
      if (field_modifier_begin)
#else
      field_modifier_begin = field_begin;
      while ((temp = memchr (field_modifier_begin, '-', field_end - field_modifier_begin)))
        field_modifier_begin = temp + 1;
      if (field_modifier_begin != field_begin)
#endif
      {
        field_modifier_size = field_modifier_end - field_modifier_begin;
      }
      else
      {
        field_modifier_begin = 0;
        field_modifier_size = 0;
      }

      value_begin = field_end + 1;
      while (value_begin < end && isspace (*++value_begin));
      readsize = end - value_begin < READSIZE ? end - value_begin : READSIZE;
      value_end = memchr (value_begin, '\n', readsize);
      if (!value_end)
      {
        di_warning ("parser_rfc822: Iek! Don't find end of value");
        return -1;
      }

      while (*(value_end + 1) == ' ' || *(value_end + 1) == '\t')
      {
        readsize = end - value_end + 1 < READSIZE ? end - value_end + 1 : READSIZE;
        if ((value_end = memchr (value_end + 1, '\n', readsize)) == NULL)
        {
          di_warning ("Iek! Don't find end of large value\n");
          return -1;
        }
      }
      value_size = value_end - value_begin;

      field_string.string = field_begin;
      field_string.size = field_size;
      value_string.string = value_begin;
      value_string.size = value_size;

      fip = di_hash_table_lookup (info->table, &field_string);

      if (fip)
        fip->read (&act, fip, NULL, &value_string, user_data);

      if (info->wildcard)
        goto wildcard;
      else if (!info->modifier)
        goto next;

      field_string.size = field_size - field_modifier_size - 1;

      fip = di_hash_table_lookup (info->table, &field_string);

      if (fip)
      {
        field_modifier_string.string = field_modifier_begin;
        field_modifier_string.size = field_modifier_size;

        fip->read (&act, fip, &field_modifier_string, &value_string, user_data);
      }

      if (!info->wildcard)
        goto next;

wildcard:
      field_string.size = 0;

      fip = di_hash_table_lookup (info->table, &field_string);
          
      if (fip)
      {
        field_modifier_string.string = field_begin;
        field_modifier_string.size = field_size;

        fip->read (&act, fip, &field_modifier_string, &value_string, user_data);
      }

next:
      cur = value_end + 1;
      if (cur >= end)
        break;
      if (*cur == '\n')
      {
        while (cur < end && *++cur == '\n');
        break;
      }
    }

    if (entry_finish && entry_finish (act, user_data))
      return -1;
  }

  return nr;
}

/**
 * Parse a rfc822 formated file
 *
 * @param file filename
 * @param info parser info
 * @param user_data user_data for parser functions
 *
 * @return number of parsed entries
 */
int di_parser_rfc822_read_file (const char *file, di_parser_info *info, di_parser_read_entry_new entry_new, di_parser_read_entry_finish entry_finish, void *user_data)
{
  struct stat statbuf;
  char *begin;
  int fd, ret;

  if ((fd = open (file, O_RDONLY)) < 0)
    return -1;
  if (fstat (fd, &statbuf))
    return -1;
  if (!(begin = mmap (NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0)))
    return -1;
  madvise (begin, statbuf.st_size, MADV_SEQUENTIAL);

  if ((ret = di_parser_rfc822_read (begin, statbuf.st_size, info, entry_new, entry_finish, user_data)) < 0)
    return -1;

  munmap (begin, statbuf.st_size);
  close (fd);

  return ret;
}

static void callback (const di_rstring *field, const di_rstring *value, void *data)
{
  FILE *f = data;
  fwrite (field->string, field->size, 1, f);
  fputs (": ", f);
  fwrite (value->string, value->size, 1, f);
  fputs ("\n", f);
}

/**
 * Dump a rfc822 formated file
 *
 * @param file filename
 * @param info parser info
 * @param user_data user_data for parser functions
 *
 * @return number of dumped entries
 */
int di_parser_rfc822_write_file (const char *file, di_parser_info *info, di_parser_write_entry_next entry_next, void *user_data)
{
  int nr = 0;
  const di_parser_fieldinfo *fip;
  void *act = NULL, *state_data = NULL;
  di_slist_node *node;
  FILE *f;


  if (!strncmp (file, "-", 1))
    f = stdout;
  else
    f = fopen (file, "w");

  if (!f)
    return -1;

  while (1)
  {
    act = entry_next (&state_data, user_data);
    if (!act)
      break;

    nr++;

    for (node = info->list.first; node; node = node->next)
    {
      fip = node->data;

      if (fip->write)
      {
        fip->write (&act, fip, callback, f, user_data);
      }
    }
    fputs ("\n", f);
  }

  if (strncmp (file, "-", 1))
    fclose (f);

  return nr;
}


