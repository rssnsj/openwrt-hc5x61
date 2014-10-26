/* pkg_extract.c - the opkg package management system

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

#ifndef PKG_EXTRACT_H
#define PKG_EXTRACT_H

#include "pkg.h"

int pkg_extract_control_file_to_stream(pkg_t *pkg, FILE *stream);
int pkg_extract_control_files_to_dir(pkg_t *pkg, const char *dir);
int pkg_extract_control_files_to_dir_with_prefix(pkg_t *pkg,
						 const char *dir,
						 const char *prefix);
int pkg_extract_data_files_to_dir(pkg_t *pkg, const char *dir);
int pkg_extract_data_file_names_to_stream(pkg_t *pkg, FILE *file);

#endif
