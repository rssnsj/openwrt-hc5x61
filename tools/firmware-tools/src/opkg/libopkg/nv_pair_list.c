/* nv_pair_list.c - the opkg package management system

   Carl D. Worth

   Copyright (C) 2001 University of Southern California

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#include "nv_pair.h"
#include "void_list.h"
#include "nv_pair_list.h"
#include "libbb/libbb.h"

void nv_pair_list_init(nv_pair_list_t *list)
{
    void_list_init((void_list_t *) list);
}

void nv_pair_list_deinit(nv_pair_list_t *list)
{
    nv_pair_list_elt_t *pos;
    nv_pair_t *nv_pair;

    while(!void_list_empty(list)) {
        pos = nv_pair_list_pop(list);
        if (!pos)
            break;
	nv_pair =  (nv_pair_t *) pos->data;
	nv_pair_deinit(nv_pair);
	/* malloced in nv_pair_list_append */
	free(nv_pair);
	pos->data = NULL;
        free(pos);
    }
    void_list_deinit((void_list_t *) list);
}

nv_pair_t *nv_pair_list_append(nv_pair_list_t *list, const char *name, const char *value)
{
    /* freed in nv_pair_list_deinit */
    nv_pair_t *nv_pair = xcalloc(1, sizeof(nv_pair_t));
    nv_pair_init(nv_pair, name, value);
    void_list_append((void_list_t *) list, nv_pair);

    return nv_pair;
}

void nv_pair_list_push(nv_pair_list_t *list, nv_pair_t *data)
{
    void_list_push((void_list_t *) list, data);
}

nv_pair_list_elt_t *nv_pair_list_pop(nv_pair_list_t *list)
{
    return (nv_pair_list_elt_t *) void_list_pop((void_list_t *) list);
}

char *nv_pair_list_find(nv_pair_list_t *list, char *name)
{
     nv_pair_list_elt_t *iter;
     nv_pair_t *nv_pair;

     list_for_each_entry(iter, &list->head, node) {
	  nv_pair = (nv_pair_t *)iter->data;
	  if (strcmp(nv_pair->name, name) == 0) {
	       return nv_pair->value;
	  }
     }
     return NULL;
}

nv_pair_list_elt_t *nv_pair_list_first(nv_pair_list_t *list) {
    return (nv_pair_list_elt_t * )void_list_first((void_list_t *) list);
}

nv_pair_list_elt_t *nv_pair_list_prev(nv_pair_list_t *list, nv_pair_list_elt_t *node) {
    return (nv_pair_list_elt_t * )void_list_prev((void_list_t *) list, (void_list_elt_t *)node);
}

nv_pair_list_elt_t *nv_pair_list_next(nv_pair_list_t *list, nv_pair_list_elt_t *node) {
    return (nv_pair_list_elt_t * )void_list_next((void_list_t *) list, (void_list_elt_t *)node);
}

nv_pair_list_elt_t *nv_pair_list_last(nv_pair_list_t *list) {
    return (nv_pair_list_elt_t * )void_list_last((void_list_t *) list);
}



