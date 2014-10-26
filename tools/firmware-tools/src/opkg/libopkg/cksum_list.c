/* cksum_lis.c - the opkg package management system

   Copyright (C) 2010,2011 Javier Palacios

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "config.h"

#include <stdio.h>

#include "cksum_list.h"
#include "libbb/libbb.h"


int cksum_init(cksum_t *cksum, char **itemlist)
{
    cksum->value = xstrdup(*itemlist++);
    cksum->size = atoi(*itemlist++);
    cksum->name = xstrdup(*itemlist++);

    return 0;
}

void cksum_deinit(cksum_t *cksum)
{
    free (cksum->name);
    cksum->name = NULL;

    free (cksum->value);
    cksum->value = NULL;
}

void cksum_list_init(cksum_list_t *list)
{
    void_list_init((void_list_t *) list);
}

void cksum_list_deinit(cksum_list_t *list)
{
    cksum_list_elt_t *iter, *n;
    cksum_t *cksum;

    list_for_each_entry_safe(iter, n, &list->head, node) {
      cksum = (cksum_t *)iter->data;
      cksum_deinit(cksum);

      /* malloced in cksum_list_append */
      free(cksum);
      iter->data = NULL;
    }
    void_list_deinit((void_list_t *) list);
}

cksum_t *cksum_list_append(cksum_list_t *list, char **itemlist)
{
    /* freed in cksum_list_deinit */
    cksum_t *cksum = xcalloc(1, sizeof(cksum_t));
    cksum_init(cksum, itemlist);

    void_list_append((void_list_t *) list, cksum);

    return cksum;
}

const cksum_t *cksum_list_find(cksum_list_t *list, const char *name)
{
     cksum_list_elt_t *iter;
     cksum_t *cksum;

     list_for_each_entry(iter, &list->head, node) {
	  cksum = (cksum_t *)iter->data;
	  if (strcmp(cksum->name, name) == 0) {
	       return cksum;
	  }
     }    
     return NULL;
}

