/* opkg_upgrade.c - the opkg package management system

   Carl D. Worth
   Copyright (C) 2001 University of Southern California

   Copyright (C) 2003 Daniele Nicolodi <daniele@grinta.net>

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
#include <stdlib.h>

#include "opkg_install.h"
#include "opkg_upgrade.h"
#include "opkg_message.h"

int
opkg_upgrade_pkg(pkg_t *old)
{
     pkg_t *new;
     int cmp;
     char *old_version, *new_version;

     if (old->state_flag & SF_HOLD) {
          opkg_msg(NOTICE, "Not upgrading package %s which is marked "
                       "hold (flags=%#x).\n", old->name, old->state_flag);
          return 0;
     }

     new = pkg_hash_fetch_best_installation_candidate_by_name(old->name);
     if (new == NULL) {
          old_version = pkg_version_str_alloc(old);
          opkg_msg(NOTICE, "Assuming locally installed package %s (%s) "
                       "is up to date.\n", old->name, old_version);
          free(old_version);
          return 0;
     }

     old_version = pkg_version_str_alloc(old);
     new_version = pkg_version_str_alloc(new);

     cmp = pkg_compare_versions(old, new);
     opkg_msg(DEBUG, "Comparing visible versions of pkg %s:"
                  "\n\t%s is installed "
                  "\n\t%s is available "
                  "\n\t%d was comparison result\n",
                  old->name, old_version, new_version, cmp);
     if (cmp == 0) {
          opkg_msg(INFO, "Package %s (%s) installed in %s is up to date.\n",
                       old->name, old_version, old->dest->name);
          free(old_version);
          free(new_version);
          return 0;
     } else if (cmp > 0) {
          opkg_msg(NOTICE, "Not downgrading package %s on %s from %s to %s.\n",
                       old->name, old->dest->name, old_version, new_version);
          free(old_version);
          free(new_version);
          return 0;
     } else if (cmp < 0) {
          new->dest = old->dest;
          old->state_want = SW_DEINSTALL;
     }

    free(old_version);
    free(new_version);
    new->state_flag |= SF_USER;
    return opkg_install_pkg(new,1);
}


static void
pkg_hash_check_installed_pkg_helper(const char *pkg_name, void *entry,
		void *data)
{
	struct active_list *head = (struct active_list *) data;
	abstract_pkg_t *ab_pkg = (abstract_pkg_t *)entry;
	pkg_vec_t *pkg_vec = ab_pkg->pkgs;
	int j;

	if (!pkg_vec)
		return;

	for (j = 0; j < pkg_vec->len; j++) {
		pkg_t *pkg = pkg_vec->pkgs[j];
		if (pkg->state_status == SS_INSTALLED
				|| pkg->state_status == SS_UNPACKED)
			active_list_add(head, &pkg->list);
	}
}

struct active_list *
prepare_upgrade_list(void)
{
    struct active_list *head = active_list_head_new();
    struct active_list *all = active_list_head_new();
    struct active_list *node=NULL;

    /* ensure all data is valid */
    pkg_info_preinstall_check();

    hash_table_foreach(&conf->pkg_hash, pkg_hash_check_installed_pkg_helper, all);
    for (node=active_list_next(all,all); node; node = active_list_next(all, node)) {
        pkg_t *old, *new;
        int cmp;

        old = list_entry(node, pkg_t, list);
        new = pkg_hash_fetch_best_installation_candidate_by_name(old->name);

        if (new == NULL)
            continue;

        cmp = pkg_compare_versions(old, new);

        if ( cmp < 0 ) {
           node = active_list_move_node(all, head, &old->list);
        }
    }
    active_list_head_delete(all);
    return head;
}
