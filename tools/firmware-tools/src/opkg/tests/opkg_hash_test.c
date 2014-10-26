/* opkg_hash_test.c - the itsy package management system

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

#include <libopkg/opkg.h>

#include <libopkg/hash_table.h>
#include <libopkg/opkg_utils.h>
#include <libopkg/pkg_hash.h>

int main(int argc, char *argv[])
{
     opkg_conf_t conf;
     hash_table_t *hash = &conf.pkg_hash;
     pkg_vec_t * pkg_vec;

    if (argc < 3) {
	fprintf(stderr, "Usage: %s <pkgs_file1> <pkgs_file2> [pkg_name...]\n", argv[0]);
	exit(1);
    }
    pkg_hash_init("test", hash, 1024);

    pkg_hash_add_from_file(&conf, argv[1], NULL, NULL, 0);
    pkg_hash_add_from_file(&conf, argv[2], NULL, NULL, 0);

    if (argc < 4) {
	pkg_print_info( pkg_hash_fetch_by_name_version(hash, "libc6", "2.2.3-2"), stdout);
	/*	for(i = 0; i < pkg_vec->len; i++)
		pkg_print(pkg_vec->pkgs[i], stdout);
	*/
    } else {
	int i, j, k;
	char **unresolved;

	pkg_vec_t * dep_vec;
	for (i = 3; i < argc; i++) {
	    pkg_vec = pkg_vec_fetch_by_name(hash, argv[i]);
	    if (pkg_vec == NULL) {
		fprintf(stderr, "*** WARNING: Unknown package: %s\n\n", argv[i]);
		continue;
	    }

	    for(j = 0; j < pkg_vec->len; j++){
		pkg_print_info(pkg_vec->pkgs[j], stdout);
		dep_vec = pkg_vec_alloc();
		pkg_hash_fetch_unsatisfied_dependencies(&conf,
							pkg_vec->pkgs[j],
							dep_vec,
							&unresolved);
		if(dep_vec){
		    fprintf(stderr, "and the unsatisfied dependencies are:\n");
		    for(k = 0; k < dep_vec->len; k++){
			fprintf(stderr, "%s version %s\n", dep_vec->pkgs[k]->name, dep_vec->pkgs[k]->version);
		    }
		}

		fputs("", stdout);

	    }
 	}
    }

    pkg_hash_deinit(hash);

    return 0;
}
