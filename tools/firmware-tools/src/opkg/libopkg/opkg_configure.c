/* opkg_configure.c - the opkg package management system

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

#include <stdio.h>

#include "sprintf_alloc.h"
#include "opkg_configure.h"
#include "opkg_message.h"
#include "opkg_cmd.h"

int
opkg_configure(pkg_t *pkg)
{
    int err;

    /* DPKG_INCOMPATIBILITY:
       dpkg actually does some conffile handling here, rather than at the
       end of opkg_install(). Do we care? */
    /* DPKG_INCOMPATIBILITY:
       dpkg actually includes a version number to this script call */

    err = pkg_run_script(pkg, "postinst", "configure");
    if (err) {
	opkg_msg(ERROR, "%s.postinst returned %d.\n", pkg->name, err);
	return err;
    }

    return 0;
}

