/* active_list.h - the opkg package management system

   Tick Chen <tick@openmoko.com>

   Copyright (C) 2008 Openmoko Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef ACTIVE_LIST_H
#define ACTIVE_LIST_H

#include "list.h"

struct active_list {
    struct list_head node;
    struct list_head depend;
    struct active_list *depended;
};


struct active_list * active_list_head_new(void);
void active_list_head_delete(struct active_list *);
void active_list_init(struct active_list *ptr);
void active_list_clear(struct active_list *head);
void active_list_add_depend(struct active_list *node, struct active_list *depend);
void active_list_add(struct active_list *head, struct active_list *node);
struct active_list *active_list_move_node(struct active_list *old_head, struct active_list *new_head, struct active_list *node);

struct active_list * active_list_sort(struct active_list *head, int (*compare_fcn_t)(const void *, const void *));

struct active_list * active_list_next(struct active_list *head, struct active_list *ptr);

struct active_list * active_list_prev(struct active_list *head, struct active_list *ptr);

#endif
