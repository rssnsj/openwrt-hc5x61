/* void_list.c - the opkg package management system

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

#include "void_list.h"
#include "libbb/libbb.h"

void void_list_elt_init(void_list_elt_t *elt, void *data)
{
    INIT_LIST_HEAD(&elt->node);
    elt->data = data;
}

static void_list_elt_t * void_list_elt_new (void *data) {
    void_list_elt_t *elt;
    /* freed in void_list_elt_deinit */
    elt = xcalloc(1, sizeof(void_list_elt_t));
    void_list_elt_init(elt, data);
    return elt;
}

void void_list_elt_deinit(void_list_elt_t *elt)
{
    list_del_init(&elt->node);
    void_list_elt_init(elt, NULL);
    free(elt);
}

void void_list_init(void_list_t *list)
{
    INIT_LIST_HEAD(&list->head);
}

void void_list_deinit(void_list_t *list)
{
    void_list_elt_t *elt;

    while (!void_list_empty(list)) {
	elt = void_list_pop(list);
	void_list_elt_deinit(elt);
    }
    INIT_LIST_HEAD(&list->head);
}

void void_list_append(void_list_t *list, void *data)
{
    void_list_elt_t *elt = void_list_elt_new(data);
    list_add_tail(&elt->node, &list->head);
}

void void_list_push(void_list_t *list, void *data)
{
    void_list_elt_t *elt = void_list_elt_new(data);
    list_add(&elt->node, &list->head);
}

void_list_elt_t *void_list_pop(void_list_t *list)
{
    struct list_head *node;

    if (void_list_empty(list))
        return NULL;
    node = list->head.next;
    list_del_init(node);
    return list_entry(node, void_list_elt_t, node);
}

void *void_list_remove(void_list_t *list, void_list_elt_t **iter)
{
    void_list_elt_t *pos, *n;
    void_list_elt_t *old_elt;
    void *old_data = NULL;

    old_elt = *iter;
    if (!old_elt)
        return old_data;
    old_data = old_elt->data;

    list_for_each_entry_safe(pos, n, &list->head, node) {
        if (pos == old_elt)
            break;
    }
    if ( pos != old_elt) {
        opkg_msg(ERROR, "Internal error: element not found in list.\n");
        return NULL;
    }

    *iter = list_entry(pos->node.prev, void_list_elt_t, node);
    void_list_elt_deinit(pos);

    return old_data;
}

/* remove element containing elt data, using cmp(elt->data, target_data) == 0. */
void *void_list_remove_elt(void_list_t *list, const void *target_data, void_list_cmp_t cmp)
{
    void_list_elt_t *pos, *n;
    void *old_data = NULL;
    list_for_each_entry_safe(pos, n, &list->head, node) {
        if ( pos->data && cmp(pos->data, target_data)==0 ){
            old_data = pos->data;
            void_list_elt_deinit(pos);
            break;
        }
    }
    return old_data;
}

void_list_elt_t *void_list_first(void_list_t *list) {
    struct list_head *elt;
    if (!list)
        return NULL;
    elt = list->head.next;
    if (elt == &list->head ) {
        return NULL;
    }
    return list_entry(elt, void_list_elt_t, node);
}

void_list_elt_t *void_list_prev(void_list_t *list, void_list_elt_t *node) {
    struct list_head *elt;
    if (!list || !node)
        return NULL;
    elt = node->node.prev;
    if (elt == &list->head ) {
        return NULL;
    }
    return list_entry(elt, void_list_elt_t, node);
}

void_list_elt_t *void_list_next(void_list_t *list, void_list_elt_t *node) {
    struct list_head *elt;
    if (!list || !node)
        return NULL;
    elt = node->node.next;
    if (elt == &list->head ) {
        return NULL;
    }
    return list_entry(elt, void_list_elt_t, node);
}

void_list_elt_t *void_list_last(void_list_t *list) {
    struct list_head *elt;
    if (!list)
        return NULL;
    elt = list->head.prev;
    if (elt == &list->head ) {
        return NULL;
    }
    return list_entry(elt, void_list_elt_t, node);
}

