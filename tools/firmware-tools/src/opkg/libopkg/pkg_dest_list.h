/* pkg_dest_list.h - the opkg package management system

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

#ifndef PKG_DEST_LIST_H
#define PKG_DEST_LIST_H

#include "pkg_dest.h"

typedef struct void_list_elt pkg_dest_list_elt_t;

typedef struct void_list pkg_dest_list_t;

void pkg_dest_list_elt_init(pkg_dest_list_elt_t *elt, pkg_dest_t *data);
void pkg_dest_list_elt_deinit(pkg_dest_list_elt_t *elt);

void pkg_dest_list_init(pkg_dest_list_t *list);
void pkg_dest_list_deinit(pkg_dest_list_t *list);

pkg_dest_t *pkg_dest_list_append(pkg_dest_list_t *list, const char *name,
				 const char *root_dir,const char* lists_dir);
void pkg_dest_list_push(pkg_dest_list_t *list, pkg_dest_t *data);
pkg_dest_list_elt_t *pkg_dest_list_pop(pkg_dest_list_t *list);

#endif

