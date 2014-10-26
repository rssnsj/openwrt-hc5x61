/* parse_util.h - the opkg package management system

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

#ifndef PARSE_UTIL_H
#define PARSE_UTIL_H

int is_field(const char *type, const char *line);
char *parse_simple(const char *type, const char *line);
char **parse_list(const char *raw, unsigned int *count, const char sep, int skip_field);

typedef int (*parse_line_t)(void *, const char *, uint);
int parse_from_stream_nomalloc(parse_line_t parse_line, void *item, FILE *fp, uint mask,
						char **buf0, size_t buf0len);

#endif
