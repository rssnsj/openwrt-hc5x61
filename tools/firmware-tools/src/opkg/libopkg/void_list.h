/* void_list.h - the opkg package management system

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

#ifndef VOID_LIST_H
#define VOID_LIST_H

#include "list.h"

typedef struct void_list_elt void_list_elt_t;
struct void_list_elt
{
    struct list_head node;
    void *data;
};

typedef struct void_list void_list_t;
struct void_list
{
    struct list_head head;
};

static inline int void_list_empty(void_list_t *list)
{
    return list_empty(&list->head);
}

void void_list_elt_init(void_list_elt_t *elt, void *data);
void void_list_elt_deinit(void_list_elt_t *elt);

void void_list_init(void_list_t *list);
void void_list_deinit(void_list_t *list);

void void_list_append(void_list_t *list, void *data);
void void_list_push(void_list_t *list, void *data);
void_list_elt_t *void_list_pop(void_list_t *list);

void *void_list_remove(void_list_t *list, void_list_elt_t **iter);
/* remove element containing elt data, using cmp(elt->data, target_data) == 0. */
typedef int (*void_list_cmp_t)(const void *, const void *);
void *void_list_remove_elt(void_list_t *list, const void *target_data, void_list_cmp_t cmp);

void_list_elt_t *void_list_first(void_list_t *list);
void_list_elt_t *void_list_prev(void_list_t *list, void_list_elt_t *node);
void_list_elt_t *void_list_next(void_list_t *list, void_list_elt_t *node);
void_list_elt_t *void_list_last(void_list_t *list);

void void_list_purge(void_list_t *list);


#endif
