/* pkg_vec.h - the opkg package management system

   Steven M. Ayer

   Copyright (C) 2002 Compaq Computer Corporation

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef PKG_VEC_H
#define PKG_VEC_H

typedef struct pkg pkg_t;
typedef struct abstract_pkg abstract_pkg_t;
typedef struct pkg_vec pkg_vec_t;
typedef struct abstract_pkg_vec abstract_pkg_vec_t;

#include "opkg_conf.h"

struct pkg_vec
{
    pkg_t **pkgs;
    unsigned int len;
};

struct abstract_pkg_vec
{
    abstract_pkg_t **pkgs;
    unsigned int len;
};


pkg_vec_t * pkg_vec_alloc(void);
void pkg_vec_free(pkg_vec_t *vec);

void pkg_vec_insert_merge(pkg_vec_t *vec, pkg_t *pkg, int set_status);
void pkg_vec_insert(pkg_vec_t *vec, const pkg_t *pkg);
int pkg_vec_contains(pkg_vec_t *vec, pkg_t *apkg);

typedef int (*compare_fcn_t)(const void *, const void *);
void pkg_vec_sort(pkg_vec_t *vec, compare_fcn_t compar);

int pkg_vec_clear_marks(pkg_vec_t *vec);
int pkg_vec_mark_if_matches(pkg_vec_t *vec, const char *pattern);

abstract_pkg_vec_t * abstract_pkg_vec_alloc(void);
void abstract_pkg_vec_free(abstract_pkg_vec_t *vec);
void abstract_pkg_vec_insert(abstract_pkg_vec_t *vec, abstract_pkg_t *pkg);
abstract_pkg_t * abstract_pkg_vec_get(abstract_pkg_vec_t *vec, int i);
int abstract_pkg_vec_contains(abstract_pkg_vec_t *vec, abstract_pkg_t *apkg);
void abstract_pkg_vec_sort(pkg_vec_t *vec, compare_fcn_t compar);

int pkg_compare_names(const void *p1, const void *p2);
#endif

