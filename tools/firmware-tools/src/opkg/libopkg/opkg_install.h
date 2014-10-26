/* opkg_install.h - the opkg package management system

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

#ifndef OPKG_INSTALL_H
#define OPKG_INSTALL_H

#include "pkg.h"
#include "opkg_conf.h"

int opkg_install_by_name(const char *pkg_name);
int opkg_install_pkg(pkg_t *pkg, int from_upgrading);

#endif
