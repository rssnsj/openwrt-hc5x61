/* conffile_list.h - the opkg package management system

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

#ifndef CONFFILE_LIST_H
#define CONFFILE_LIST_H

#include "nv_pair_list.h"

typedef nv_pair_list_elt_t conffile_list_elt_t;
typedef nv_pair_list_t conffile_list_t;

#include "conffile.h"

void conffile_list_init(conffile_list_t *list);
void conffile_list_deinit(conffile_list_t *list);

conffile_t *conffile_list_append(conffile_list_t *list, const char *name,
			       const char *root_dir);
void conffile_list_push(conffile_list_t *list, conffile_t *data);
conffile_list_elt_t *conffile_list_pop(conffile_list_t *list);

#endif

