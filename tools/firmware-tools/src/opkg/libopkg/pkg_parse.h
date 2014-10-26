/* pkg_parse.h - the opkg package management system

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

#ifndef PKG_PARSE_H
#define PKG_PARSE_H

#include "pkg.h"

int parse_version(pkg_t *pkg, const char *raw);
int pkg_parse_from_stream(pkg_t *pkg, FILE *fp, uint mask);
int pkg_parse_line(void *ptr, const char *line, uint mask);

#define EXCESSIVE_LINE_LEN	(4096 << 8)

/* package field mask */
#define PFM_ARCHITECTURE	(1 << 1)
#define PFM_AUTO_INSTALLED	(1 << 2)
#define PFM_CONFFILES		(1 << 3)
#define PFM_CONFLICTS		(1 << 4)
#define PFM_DESCRIPTION		(1 << 5)
#define PFM_DEPENDS		(1 << 6)
#define PFM_ESSENTIAL		(1 << 7)
#define PFM_FILENAME		(1 << 8)
#define PFM_INSTALLED_SIZE	(1 << 9)
#define PFM_INSTALLED_TIME	(1 << 10)
#define PFM_MD5SUM		(1 << 11)
#define PFM_MAINTAINER		(1 << 12)
#define PFM_PACKAGE		(1 << 13)
#define PFM_PRIORITY		(1 << 14)
#define PFM_PROVIDES		(1 << 15)
#define PFM_PRE_DEPENDS		(1 << 16)
#define PFM_RECOMMENDS		(1 << 17)
#define PFM_REPLACES		(1 << 18)
#define PFM_SECTION		(1 << 19)
#define PFM_SHA256SUM		(1 << 20)
#define PFM_SIZE		(1 << 21)
#define PFM_SOURCE		(1 << 22)
#define PFM_STATUS		(1 << 23)
#define PFM_SUGGESTS		(1 << 24)
#define PFM_TAGS		(1 << 25)
#define PFM_VERSION		(1 << 26)

#define PFM_ALL	(~(uint)0)

#endif
