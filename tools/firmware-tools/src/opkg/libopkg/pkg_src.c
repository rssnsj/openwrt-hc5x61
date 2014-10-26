/* pkg_src.c - the opkg package management system

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

#include "pkg_src.h"
#include "libbb/libbb.h"

int pkg_src_init(pkg_src_t *src, const char *name, const char *base_url, const char *extra_data, int gzip)
{
    src->gzip = gzip;
    src->name = xstrdup(name);
    src->value = xstrdup(base_url);
    if (extra_data)
	src->extra_data = xstrdup(extra_data);
    else
	src->extra_data = NULL;
    return 0;
}

void pkg_src_deinit(pkg_src_t *src)
{
    free (src->name);
    free (src->value);
    if (src->extra_data)
	free (src->extra_data);
}
