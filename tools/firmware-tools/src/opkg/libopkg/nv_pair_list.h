/* nv_pair_list.h - the opkg package management system

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

#ifndef NV_PAIR_LIST_H
#define NV_PAIR_LIST_H

#include "nv_pair.h"
#include "void_list.h"

typedef struct void_list_elt nv_pair_list_elt_t;

typedef struct void_list nv_pair_list_t;

static inline int nv_pair_list_empty(nv_pair_list_t *list)
{
    return void_list_empty ((void_list_t *)list);
}

void nv_pair_list_init(nv_pair_list_t *list);
void nv_pair_list_deinit(nv_pair_list_t *list);

nv_pair_t *nv_pair_list_append(nv_pair_list_t *list,
			       const char *name, const char *value);
void nv_pair_list_push(nv_pair_list_t *list, nv_pair_t *data);
nv_pair_list_elt_t *nv_pair_list_pop(nv_pair_list_t *list);
char *nv_pair_list_find(nv_pair_list_t *list, char *name);

nv_pair_list_elt_t *nv_pair_list_first(nv_pair_list_t *list);
nv_pair_list_elt_t *nv_pair_list_prev(nv_pair_list_t *list, nv_pair_list_elt_t *node);
nv_pair_list_elt_t *nv_pair_list_next(nv_pair_list_t *list, nv_pair_list_elt_t *node);
nv_pair_list_elt_t *nv_pair_list_last(nv_pair_list_t *list);

#endif

