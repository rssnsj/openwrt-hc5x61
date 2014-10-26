/* conffile.h - the opkg package management system

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

#ifndef CONFFILE_H
#define CONFFILE_H

#include "nv_pair.h"
typedef struct nv_pair conffile_t;

int conffile_init(conffile_t *conffile, const char *file_name, const char *md5sum);
void conffile_deinit(conffile_t *conffile);
int conffile_has_been_modified(conffile_t *conffile);

#endif

