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


#include "active_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libbb/libbb.h"

void active_list_init(struct active_list *ptr) {
    INIT_LIST_HEAD(&ptr->node);
    INIT_LIST_HEAD(&ptr->depend);
    ptr->depended = NULL;
}

/**
 */
struct active_list * active_list_next(struct active_list *head, struct active_list *ptr) {
    struct active_list *next=NULL;
    if ( !head ) {
        opkg_msg(ERROR, "Internal error: head=%p, ptr=%p\n", head, ptr);
        return NULL;
    }
    if ( !ptr )
        ptr = head;
    next = list_entry(ptr->node.next, struct active_list, node);
    if ( next == head ) {
        return NULL;
    }
    if ( ptr->depended && &ptr->depended->depend == ptr->node.next ) {
        return ptr->depended;
    }
    while ( next->depend.next != &next->depend ) {
        next = list_entry(next->depend.next, struct active_list, node);
    }
    return next;
}


struct active_list * active_list_prev(struct active_list *head, struct active_list *ptr) {
    struct active_list *prev=NULL;
    if ( !head ) {
        opkg_msg(ERROR, "Internal error: head=%p, ptr=%p\n", head, ptr);
        return NULL;
    }
    if ( !ptr )
        ptr = head;
    if ( ptr->depend.prev != &ptr->depend ) {
        prev = list_entry(ptr->depend.prev, struct active_list, node);
        return prev;
    }
    if ( ptr->depended  && ptr->depended != head && &ptr->depended->depend == ptr->node.prev ) {
        prev = list_entry(ptr->depended->node.prev, struct active_list, node);
    } else
        prev = list_entry(ptr->node.prev, struct active_list, node);
    if ( prev == head )
        return NULL;
    return prev;
}


struct active_list *active_list_move_node(struct active_list *old_head, struct active_list *new_head, struct active_list *node) {
    struct active_list *prev;
    if (!old_head || !new_head || !node)
        return NULL;
    if (old_head == new_head)
        return node;
    prev = active_list_prev(old_head, node);
    active_list_add(new_head, node);
    return prev;
}

static void list_head_clear (struct list_head *head) {
    struct active_list *next;
    struct list_head *n, *ptr;
    if (!head)
        return;
    list_for_each_safe(ptr, n , head) {
        next = list_entry(ptr, struct active_list, node);
        if (next->depend.next != &next->depend) {
            list_head_clear(&next->depend);
        }
        active_list_init(next);
    }
}
void active_list_clear(struct active_list *head) {
    list_head_clear(&head->node);
    if (head->depend.next != &head->depend) {
        list_head_clear(&head->depend);
    }
    active_list_init(head);
}

void active_list_add_depend(struct active_list *node, struct active_list *depend) {
    list_del_init(&depend->node);
    list_add_tail(&depend->node, &node->depend);
    depend->depended  = node;
}

void active_list_add(struct active_list *head, struct active_list *node) {
    list_del_init(&node->node);
    list_add_tail(&node->node, &head->node);
    node->depended  = head;
}

struct active_list * active_list_head_new(void) {
    struct active_list * head = xcalloc(1, sizeof(struct active_list));
    active_list_init(head);
    return head;
}

void active_list_head_delete(struct active_list *head) {
    active_list_clear(head);
    free(head);
}

/*
 *  Using insert sort.
 *  Note. the list should not be large, or it will be very inefficient.
 *
 */
struct active_list * active_list_sort(struct active_list *head, int (*compare)(const void *, const void *)) {
    struct active_list tmphead;
    struct active_list *node, *ptr;
    if ( !head )
        return NULL;
    active_list_init(&tmphead);
    for (node = active_list_next(head, NULL); node; node = active_list_next(head, NULL)) {
        if (tmphead.node.next == &tmphead.node) {
            active_list_move_node(head, &tmphead, node);
        } else {
            for (ptr = active_list_next(&tmphead, NULL); ptr; ptr=active_list_next(&tmphead, ptr)) {
                if (compare(ptr, node) <= 0) {
                    break;
                }
            }
            if (!ptr) {
                active_list_move_node(head, &tmphead, node);
            } else {
                active_list_move_node(head, ptr, node);
            }
        }
        node->depended = &tmphead;
    }
    for (ptr = active_list_prev(&tmphead, NULL); ptr; ptr=active_list_prev(&tmphead, NULL)) {
        active_list_move_node(&tmphead, head, ptr);
    }
    return head;
}
