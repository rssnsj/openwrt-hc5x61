/* xregex.h - regex functions with error messages

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

#ifndef XREGEX_H
#define XREGEX_H

#include <sys/types.h>
#include <regex.h>

int xregcomp(regex_t *preg, const char *regex, int cflags);
static inline void xregfree(regex_t *preg)
{
     regfree(preg);
}


#endif
