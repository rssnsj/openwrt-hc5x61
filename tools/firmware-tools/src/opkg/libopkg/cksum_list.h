/* cksum_list.h - the opkg package management system

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

#ifndef CKSUM_LIST_H
#define CKSUM_LIST_H

typedef struct 
{
  char *name;
  char *value;
  int size;
} cksum_t;

int cksum_init(cksum_t *cksum, char **itemlist);
void cksum_deinit(cksum_t *cksum);

#include "void_list.h"

typedef struct void_list_elt cksum_list_elt_t;

typedef struct void_list cksum_list_t;

static inline int cksum_list_empty(cksum_list_t *list)
{
    return void_list_empty ((void_list_t *)list);
}

void cksum_list_init(cksum_list_t *list);
void cksum_list_deinit(cksum_list_t *list);

cksum_t *cksum_list_append(cksum_list_t *list, char **itemlist);
const cksum_t *cksum_list_find(cksum_list_t *list, const char *name);

#endif
