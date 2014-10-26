/* pkg_extract.c - the opkg package management system

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

#include "pkg_extract.h"
#include "libbb/libbb.h"
#include "file_util.h"
#include "sprintf_alloc.h"

int
pkg_extract_control_file_to_stream(pkg_t *pkg, FILE *stream)
{
	int err;
	deb_extract(pkg->local_filename, stream,
			extract_control_tar_gz
			| extract_to_stream,
			NULL, "control", &err);
	return err;
}

int
pkg_extract_control_files_to_dir_with_prefix(pkg_t *pkg, const char *dir,
		const char *prefix)
{
	int err;
	char *dir_with_prefix;

	sprintf_alloc(&dir_with_prefix, "%s/%s", dir, prefix);

	deb_extract(pkg->local_filename, stderr,
			extract_control_tar_gz
			| extract_all_to_fs| extract_preserve_date
			| extract_unconditional,
			dir_with_prefix, NULL, &err);

	free(dir_with_prefix);
	return err;
}

int
pkg_extract_control_files_to_dir(pkg_t *pkg, const char *dir)
{
	return pkg_extract_control_files_to_dir_with_prefix(pkg, dir, "");
}


int
pkg_extract_data_files_to_dir(pkg_t *pkg, const char *dir)
{
	int err;

	deb_extract(pkg->local_filename, stderr,
		extract_data_tar_gz
		| extract_all_to_fs| extract_preserve_date
		| extract_unconditional,
		dir, NULL, &err);

	return err;
}

int
pkg_extract_data_file_names_to_stream(pkg_t *pkg, FILE *stream)
{
	int err;

    /* XXX: DPKG_INCOMPATIBILITY: deb_extract will extract all of the
       data file names with a '.' as the first character. I've taught
       opkg how to cope with the presence or absence of the '.', but
       this may trip up dpkg.

       For all I know, this could actually be a bug in opkg-build. So,
       I'll have to try installing some .debs and comparing the *.list
       files.

       If we wanted to, we could workaround the deb_extract behavior
       right here, by writing to a tmpfile, then munging things as we
       wrote to the actual stream. */

	deb_extract(pkg->local_filename, stream,
		extract_quiet | extract_data_tar_gz | extract_list,
		NULL, NULL, &err);

	return err;
}
