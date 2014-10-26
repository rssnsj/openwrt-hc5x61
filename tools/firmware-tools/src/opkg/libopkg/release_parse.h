/* release_parse.h - the opkg package management system

   Copyright (C) 2010,2011 Javier Palacios

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef RELEASE_PARSE_H
#define RELEASE_PARSE_H

int release_parse_from_stream(release_t *release, FILE *fp);

#endif
