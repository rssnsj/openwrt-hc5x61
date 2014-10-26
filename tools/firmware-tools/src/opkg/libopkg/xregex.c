/* xregex.c - regex functions with error messages

   Carl D. Worth

   Copyright (C) 2001 University of Southern California

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include "xregex.h"
#include "libbb/libbb.h"

static void print_regcomp_err(const regex_t *preg, int err);

int xregcomp(regex_t *preg, const char *regex, int cflags)
{
    int err;
    err = regcomp(preg, regex, cflags);
    if (err) {
	print_regcomp_err(preg, err);
    }

    return err;
}

static void print_regcomp_err(const regex_t *preg, int err)
{
    unsigned int size;
    char *error;

    size = regerror(err, preg, 0, 0);
    error = xcalloc(1, size);
    regerror(err, preg, error, size);

    opkg_msg(ERROR, "Internal error compiling regex: %s.", error);

    free(error);
}
