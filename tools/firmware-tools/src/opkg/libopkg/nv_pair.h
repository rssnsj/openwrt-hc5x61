/* nv_pair.h - the opkg package management system

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

#ifndef NV_PAIR_H
#define NV_PAIR_H

typedef struct nv_pair nv_pair_t;
struct nv_pair
{
    char *name;
    char *value;
};

int nv_pair_init(nv_pair_t *nv_pair, const char *name, const char *value);
void nv_pair_deinit(nv_pair_t *nv_pair);

#endif

