/* pkg_src.h - the opkg package management system

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

#ifndef PKG_SRC_H
#define PKG_SRC_H

#include "nv_pair.h"

typedef struct
{
  char *name;
  char *value;
  char *extra_data;
  int gzip;
} pkg_src_t;

int pkg_src_init(pkg_src_t *src, const char *name, const char *base_url, const char *extra_data, int gzip);
void pkg_src_deinit(pkg_src_t *src);

#endif
