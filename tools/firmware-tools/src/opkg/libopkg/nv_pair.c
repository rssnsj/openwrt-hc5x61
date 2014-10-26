/* nv_pair.c - the opkg package management system

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

#include "nv_pair.h"
#include "libbb/libbb.h"

int nv_pair_init(nv_pair_t *nv_pair, const char *name, const char *value)
{

    nv_pair->name = xstrdup(name);
    nv_pair->value = xstrdup(value);

    return 0;
}

void nv_pair_deinit(nv_pair_t *nv_pair)
{
    free(nv_pair->name);
    nv_pair->name = NULL;

    free(nv_pair->value);
    nv_pair->value = NULL;
}


