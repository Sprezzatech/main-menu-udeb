/*
 * slist.c
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
 * $Id: slist.c,v 1.4 2003/11/19 09:24:14 waldi Exp $
 */

#include <debian-installer/slist.h>

#include <debian-installer/mem.h>

di_slist *di_slist_alloc (void)
{
  di_slist *slist;

  slist = di_new0 (di_slist, 1);

  return slist;
}

void di_slist_destroy (di_slist *slist, di_destroy_notify destroy_func)
{
  di_slist_node *node, *temp;

  node = slist->head;
  while (node)
  {
    temp = node;
    node = node->next;
    if (destroy_func)
      destroy_func (temp->data);
    di_free (temp);
  }
}

void di_slist_free (di_slist *slist)
{
  di_free (slist);
}

void internal_di_slist_append (di_slist *slist, void *data, di_slist_node *new_node)
{
  new_node->data = data;
  new_node->next = NULL;

  if (slist->bottom)
    slist->bottom->next = new_node;
  else
    slist->head = new_node;

  slist->bottom = new_node;
}

void di_slist_append (di_slist *slist, void *data)
{
  return internal_di_slist_append (slist, data, di_new (di_slist_node, 1));
}

void di_slist_append_chunk (di_slist *slist, void *data, di_mem_chunk *mem_chunk)
{
  return internal_di_slist_append (slist, data, di_mem_chunk_alloc (mem_chunk));
}

void internal_di_slist_prepend (di_slist *slist, void *data, di_slist_node *new_node)
{
  new_node->data = data;
  new_node->next = slist->head;

  if (!new_node->next)
    slist->bottom = new_node;

  slist->head = new_node;
}

void di_slist_prepend (di_slist *slist, void *data)
{
  return internal_di_slist_prepend (slist, data, di_new (di_slist_node, 1));
}

void di_slist_prepend_chunk (di_slist *slist, void *data, di_mem_chunk *mem_chunk)
{
  return internal_di_slist_prepend (slist, data, di_mem_chunk_alloc (mem_chunk));
}

