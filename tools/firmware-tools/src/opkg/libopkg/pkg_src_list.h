/* pkg_src_list.h - the opkg package management system

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

#ifndef PKG_SRC_LIST_H
#define PKG_SRC_LIST_H

#include "pkg_src.h"
#include "void_list.h"

typedef struct void_list_elt pkg_src_list_elt_t;

typedef struct void_list pkg_src_list_t;

static inline int pkg_src_list_empty(pkg_src_list_t *list)
{
    return void_list_empty((void_list_t *)list);
}

void pkg_src_list_elt_init(pkg_src_list_elt_t *elt, nv_pair_t *data);
void pkg_src_list_elt_deinit(pkg_src_list_elt_t *elt);

void pkg_src_list_init(pkg_src_list_t *list);
void pkg_src_list_deinit(pkg_src_list_t *list);

pkg_src_t *pkg_src_list_append(pkg_src_list_t *list, const char *name, const char *root_dir, const char *extra_data, int gzip);
void pkg_src_list_push(pkg_src_list_t *list, pkg_src_t *data);
pkg_src_list_elt_t *pkg_src_list_pop(pkg_src_list_t *list);

#endif

