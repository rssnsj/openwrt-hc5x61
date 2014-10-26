/* opkg_install.c - the opkg package management system

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

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include "pkg.h"
#include "pkg_hash.h"
#include "pkg_extract.h"

#include "opkg_install.h"
#include "opkg_configure.h"
#include "opkg_download.h"
#include "opkg_remove.h"

#include "opkg_utils.h"
#include "opkg_message.h"
#include "opkg_cmd.h"
#include "opkg_defines.h"

#include "sprintf_alloc.h"
#include "file_util.h"
#include "xsystem.h"
#include "libbb/libbb.h"

static int
satisfy_dependencies_for(pkg_t *pkg)
{
     int i, err;
     pkg_vec_t *depends = pkg_vec_alloc();
     pkg_t *dep;
     char **tmp, **unresolved = NULL;
     int ndepends;

     ndepends = pkg_hash_fetch_unsatisfied_dependencies(pkg, depends,
							&unresolved);

     if (unresolved) {
	  opkg_msg(ERROR, "Cannot satisfy the following dependencies for %s:\n",
		       pkg->name);
	  tmp = unresolved;
	  while (*unresolved) {
	       opkg_message(ERROR, "\t%s", *unresolved);
	       free(*unresolved);
	       unresolved++;
	  }
	  free(tmp);
	  opkg_message(ERROR, "\n");
	  if (! conf->force_depends) {
	       opkg_msg(INFO,
			    "This could mean that your package list is out of date or that the packages\n"
			    "mentioned above do not yet exist (try 'opkg update'). To proceed in spite\n"
			    "of this problem try again with the '-force-depends' option.\n");
	       pkg_vec_free(depends);
	       return -1;
	  }
     }

     if (ndepends <= 0) {
	  pkg_vec_free(depends);
	  return 0;
     }

     /* Mark packages as to-be-installed */
     for (i=0; i < depends->len; i++) {
	  /* Dependencies should be installed the same place as pkg */
	  if (depends->pkgs[i]->dest == NULL) {
	       depends->pkgs[i]->dest = pkg->dest;
	  }
	  depends->pkgs[i]->state_want = SW_INSTALL;
     }

     for (i = 0; i < depends->len; i++) {
	  dep = depends->pkgs[i];
	  /* The package was uninstalled when we started, but another
	     dep earlier in this loop may have depended on it and pulled
	     it in, so check first. */
	  if ((dep->state_status != SS_INSTALLED)
	      && (dep->state_status != SS_UNPACKED)) {
               opkg_msg(DEBUG2,"Calling opkg_install_pkg.\n");
	       err = opkg_install_pkg(dep, 0);
	       /* mark this package as having been automatically installed to
	        * satisfy a dependancy */
	       dep->auto_installed = 1;
	       if (err) {
		    pkg_vec_free(depends);
		    return err;
	       }
	  }
     }

     pkg_vec_free(depends);

     return 0;
}

static int
check_conflicts_for(pkg_t *pkg)
{
     int i;
     pkg_vec_t *conflicts = NULL;
     message_level_t level;

     if (conf->force_depends) {
	  level = NOTICE;
     } else {
	  level = ERROR;
     }

     if (!conf->force_depends)
	  conflicts = pkg_hash_fetch_conflicts(pkg);

     if (conflicts) {
	  opkg_msg(level, "The following packages conflict with %s:\n",
		       pkg->name);
	  i = 0;
	  while (i < conflicts->len)
	       opkg_msg(level, "\t%s", conflicts->pkgs[i++]->name);
	  opkg_message(level, "\n");
	  pkg_vec_free(conflicts);
	  return -1;
     }
     return 0;
}

static int
update_file_ownership(pkg_t *new_pkg, pkg_t *old_pkg)
{
     str_list_t *new_list, *old_list;
     str_list_elt_t *iter, *niter;

     new_list = pkg_get_installed_files(new_pkg);
     if (new_list == NULL)
	     return -1;

     for (iter = str_list_first(new_list), niter = str_list_next(new_list, iter);
             iter;
             iter = niter, niter = str_list_next(new_list, niter)) {
	  char *new_file = (char *)iter->data;
	  pkg_t *owner = file_hash_get_file_owner(new_file);
	  pkg_t *obs = hash_table_get(&conf->obs_file_hash, new_file);

	  opkg_msg(DEBUG2, "%s: new_pkg=%s wants file %s, from owner=%s\n",
		__func__, new_pkg->name, new_file, owner?owner->name:"<NULL>");

	  if (!owner || (owner == old_pkg) || obs)
	       file_hash_set_file_owner(new_file, new_pkg);
     }

     if (old_pkg) {
	  old_list = pkg_get_installed_files(old_pkg);
	  if (old_list == NULL) {
     		  pkg_free_installed_files(new_pkg);
		  return -1;
	  }

	  for (iter = str_list_first(old_list), niter = str_list_next(old_list, iter);
                  iter;
                  iter = niter, niter = str_list_next(old_list, niter)) {
	       char *old_file = (char *)iter->data;
	       pkg_t *owner = file_hash_get_file_owner(old_file);
	       if (!owner || (owner == old_pkg)) {
		    /* obsolete */
		    hash_table_insert(&conf->obs_file_hash, old_file, old_pkg);
	       }
	  }
          pkg_free_installed_files(old_pkg);
     }
     pkg_free_installed_files(new_pkg);
     return 0;
}

static int
verify_pkg_installable(pkg_t *pkg)
{
	unsigned long kbs_available, pkg_size_kbs;
	char *root_dir = NULL;
	struct stat s;

	if (conf->force_space || pkg->installed_size == 0)
		return 0;

	if (pkg->dest)
	{
		if (!strcmp(pkg->dest->name, "root") && conf->overlay_root
		    && !stat(conf->overlay_root, &s) && (s.st_mode & S_IFDIR))
			root_dir = conf->overlay_root;
		else
			root_dir = pkg->dest->root_dir;
	}

	if (!root_dir)
		root_dir = conf->default_dest->root_dir;

	kbs_available = get_available_kbytes(root_dir);

	pkg_size_kbs = (pkg->installed_size + 1023)/1024;

	if (pkg_size_kbs >= kbs_available) {
		opkg_msg(ERROR, "Only have %ldkb available on filesystem %s, "
			"pkg %s needs %ld\n",
			kbs_available, root_dir, pkg->name, pkg_size_kbs);
		return -1;
	}

     return 0;
}

static int
unpack_pkg_control_files(pkg_t *pkg)
{
     int err;
     char *conffiles_file_name;
     char *root_dir;
     FILE *conffiles_file;

     sprintf_alloc(&pkg->tmp_unpack_dir, "%s/%s-XXXXXX", conf->tmp_dir, pkg->name);

     pkg->tmp_unpack_dir = mkdtemp(pkg->tmp_unpack_dir);
     if (pkg->tmp_unpack_dir == NULL) {
	  opkg_perror(ERROR, "Failed to create temporary directory '%s'",
		       pkg->tmp_unpack_dir);
	  return -1;
     }

     err = pkg_extract_control_files_to_dir(pkg, pkg->tmp_unpack_dir);
     if (err) {
	  return err;
     }

     /* XXX: CLEANUP: There might be a cleaner place to read in the
	conffiles. Seems like I should be able to get everything to go
	through pkg_init_from_file. If so, maybe it would make sense to
	move all of unpack_pkg_control_files to that function. */

     /* Don't need to re-read conffiles if we already have it */
     if (!nv_pair_list_empty(&pkg->conffiles)) {
	  return 0;
     }

     sprintf_alloc(&conffiles_file_name, "%s/conffiles", pkg->tmp_unpack_dir);
     if (! file_exists(conffiles_file_name)) {
	  free(conffiles_file_name);
	  return 0;
     }

     conffiles_file = fopen(conffiles_file_name, "r");
     if (conffiles_file == NULL) {
	  opkg_perror(ERROR, "Failed to open %s", conffiles_file_name);
	  free(conffiles_file_name);
	  return -1;
     }
     free(conffiles_file_name);

     while (1) {
	  char *cf_name;
	  char *cf_name_in_dest;
	  int i;

	  cf_name = file_read_line_alloc(conffiles_file);
	  if (cf_name == NULL) {
	       break;
	  }
	  if (cf_name[0] == '\0') {
	       continue;
	  }
	  for (i = strlen(cf_name) - 1;
	       (i >= 0) && (cf_name[i] == ' ' || cf_name[i] == '\t');
	       i--
	  ) {
	       cf_name[i] = '\0';
	  }

	  /* Prepend dest->root_dir to conffile name.
	     Take pains to avoid multiple slashes. */
	  root_dir = pkg->dest->root_dir;
	  if (conf->offline_root)
	       /* skip the offline_root prefix */
	       root_dir = pkg->dest->root_dir + strlen(conf->offline_root);
	  sprintf_alloc(&cf_name_in_dest, "%s%s", root_dir,
			cf_name[0] == '/' ? (cf_name + 1) : cf_name);

	  /* Can't get an md5sum now, (file isn't extracted yet).
	     We'll wait until resolve_conffiles */
	  conffile_list_append(&pkg->conffiles, cf_name_in_dest, NULL);

	  free(cf_name);
	  free(cf_name_in_dest);
     }

     fclose(conffiles_file);

     return 0;
}

/*
 * Remove packages which were auto_installed due to a dependency by old_pkg,
 * which are no longer a dependency in the new (upgraded) pkg.
 */
static int
pkg_remove_orphan_dependent(pkg_t *pkg, pkg_t *old_pkg)
{
	int i, j, k, l, found,r, err = 0;
	int n_deps;
	pkg_t *p;
	struct compound_depend *cd0, *cd1;
        abstract_pkg_t **dependents;

	int count0 = old_pkg->pre_depends_count +
				old_pkg->depends_count +
				old_pkg->recommends_count +
				old_pkg->suggests_count;
	int count1 = pkg->pre_depends_count +
				pkg->depends_count +
				pkg->recommends_count +
				pkg->suggests_count;

	for (i=0; i<count0; i++) {
		cd0 = &old_pkg->depends[i];
		if (cd0->type != DEPEND)
			continue;
		for (j=0; j<cd0->possibility_count; j++) {

			found = 0;

			for (k=0; k<count1; k++) {
				cd1 = &pkg->depends[k];
				if (cd1->type != DEPEND)
					continue;
				for (l=0; l<cd1->possibility_count; l++) {
					if (cd0->possibilities[j]
					 == cd1->possibilities[l]) {
						found = 1;
						break;
					}
				}
				if (found)
					break;
			}

			if (found)
				continue;

			/*
			 * old_pkg has a dependency that pkg does not.
			 */
			p = pkg_hash_fetch_installed_by_name(
					cd0->possibilities[j]->pkg->name);

			if (!p)
				continue;

			if (!p->auto_installed)
				continue;

			n_deps = pkg_has_installed_dependents(p, &dependents);
			n_deps--; /* don't count old_pkg */

			if (n_deps == 0) {
				opkg_msg(NOTICE, "%s was autoinstalled and is "
						"now orphaned, removing.\n",
						p->name);

				/* p has one installed dependency (old_pkg),
				 * which we need to ignore during removal. */
				p->state_flag |= SF_REPLACE;

				r = opkg_remove_pkg(p, 0);
				if (!err)
					err = r;
			} else
				opkg_msg(INFO, "%s was autoinstalled and is "
						"still required by %d "
						"installed packages.\n",
						p->name, n_deps);

		}
	}

	return err;
}

/* returns number of installed replacees */
static int
pkg_get_installed_replacees(pkg_t *pkg, pkg_vec_t *installed_replacees)
{
     abstract_pkg_t **replaces = pkg->replaces;
     int replaces_count = pkg->replaces_count;
     int i, j;
     for (i = 0; i < replaces_count; i++) {
	  abstract_pkg_t *ab_pkg = replaces[i];
	  pkg_vec_t *pkg_vec = ab_pkg->pkgs;
	  if (pkg_vec) {
	       for (j = 0; j < pkg_vec->len; j++) {
		    pkg_t *replacee = pkg_vec->pkgs[j];
		    if (!pkg_conflicts(pkg, replacee))
			 continue;
		    if (replacee->state_status == SS_INSTALLED) {
			 pkg_vec_insert(installed_replacees, replacee);
		    }
	       }
	  }
     }
     return installed_replacees->len;
}

static int
pkg_remove_installed_replacees(pkg_vec_t *replacees)
{
     int i;
     int replaces_count = replacees->len;
     for (i = 0; i < replaces_count; i++) {
	  pkg_t *replacee = replacees->pkgs[i];
	  int err;
	  replacee->state_flag |= SF_REPLACE; /* flag it so remove won't complain */
	  err = opkg_remove_pkg(replacee, 0);
	  if (err)
	       return err;
     }
     return 0;
}

/* to unwind the removal: make sure they are installed */
static int
pkg_remove_installed_replacees_unwind(pkg_vec_t *replacees)
{
     int i, err;
     int replaces_count = replacees->len;
     for (i = 0; i < replaces_count; i++) {
	  pkg_t *replacee = replacees->pkgs[i];
	  if (replacee->state_status != SS_INSTALLED) {
               opkg_msg(DEBUG2, "Calling opkg_install_pkg.\n");
	       err = opkg_install_pkg(replacee, 0);
	       if (err)
		    return err;
	  }
     }
     return 0;
}

/* compares versions of pkg and old_pkg, returns 0 if OK to proceed with installation of pkg, 1 otherwise */
static int
opkg_install_check_downgrade(pkg_t *pkg, pkg_t *old_pkg, int message)
{
     if (old_pkg) {
          char message_out[15];
	  char *old_version = pkg_version_str_alloc(old_pkg);
	  char *new_version = pkg_version_str_alloc(pkg);
	  int cmp = pkg_compare_versions(old_pkg, pkg);
	  int rc = 0;

          memset(message_out,'\x0',15);
          strncpy (message_out,"Upgrading ",strlen("Upgrading "));
          if ( (conf->force_downgrade==1) && (cmp > 0) ){     /* We've been asked to allow downgrade  and version is precedent */
             cmp = -1 ;                                       /* then we force opkg to downgrade */
             strncpy (message_out,"Downgrading ",strlen("Downgrading "));         /* We need to use a value < 0 because in the 0 case we are asking to */
                                                              /* reinstall, and some check could fail asking the "force-reinstall" option */
          }

	  if (cmp > 0) {
              if(!conf->download_only)
                  opkg_msg(NOTICE,
                          "Not downgrading package %s on %s from %s to %s.\n",
                          old_pkg->name, old_pkg->dest->name, old_version, new_version);
	       rc = 1;
	  } else if (cmp < 0) {
              if(!conf->download_only)
                  opkg_msg(NOTICE, "%s%s on %s from %s to %s...\n",
                          message_out, pkg->name, old_pkg->dest->name, old_version, new_version);
	       pkg->dest = old_pkg->dest;
	       rc = 0;
	  } else /* cmp == 0 */ {
               if(!conf->download_only)
                   opkg_msg(NOTICE, "%s (%s) already install on %s.\n",
			pkg->name, new_version, old_pkg->dest->name);
	       rc = 1;
	  }
	  free(old_version);
	  free(new_version);
	  return rc;
     } else {
          char message_out[15] ;
          memset(message_out,'\x0',15);
          if ( message )
               strncpy( message_out,"Upgrading ",strlen("Upgrading ") );
          else
               strncpy( message_out,"Installing ",strlen("Installing ") );
          char *version = pkg_version_str_alloc(pkg);

          if(!conf->download_only)
               opkg_msg(NOTICE, "%s%s (%s) to %s...\n", message_out,
                  pkg->name, version, pkg->dest->name);
          free(version);
     }
     return 0;
}


static int
prerm_upgrade_old_pkg(pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?

	1. If a version of the package is already installed, call
	   old-prerm upgrade new-version
	2. If the script runs but exits with a non-zero exit status
	   new-prerm failed-upgrade old-version
	   Error unwind, for both the above cases:
	   old-postinst abort-upgrade new-version
     */
     return 0;
}

static int
prerm_upgrade_old_pkg_unwind(pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	(See prerm_upgrade_old_package for details)
     */
     return 0;
}

static int
prerm_deconfigure_conflictors(pkg_t *pkg, pkg_vec_t *conflictors)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	2. If a 'conflicting' package is being removed at the same time:
		1. If any packages depended on that conflicting package and
		   --auto-deconfigure is specified, call, for each such package:
		   deconfigured's-prerm deconfigure \
		   in-favour package-being-installed version \
		   removing conflicting-package version
		Error unwind:
		   deconfigured's-postinst abort-deconfigure \
		   in-favour package-being-installed-but-failed version \
		   removing conflicting-package version

		   The deconfigured packages are marked as requiring
		   configuration, so that if --install is used they will be
		   configured again if possible.
		2. To prepare for removal of the conflicting package, call:
		   conflictor's-prerm remove in-favour package new-version
		Error unwind:
		   conflictor's-postinst abort-remove in-favour package new-version
     */
     return 0;
}

static int
prerm_deconfigure_conflictors_unwind(pkg_t *pkg, pkg_vec_t *conflictors)
{
     /* DPKG_INCOMPATIBILITY: dpkg does some things here that we don't
	do yet. Do we care?  (See prerm_deconfigure_conflictors for
	details) */
     return 0;
}

static int
preinst_configure(pkg_t *pkg, pkg_t *old_pkg)
{
     int err;
     char *preinst_args;

     if (old_pkg) {
	  char *old_version = pkg_version_str_alloc(old_pkg);
	  sprintf_alloc(&preinst_args, "upgrade %s", old_version);
	  free(old_version);
     } else if (pkg->state_status == SS_CONFIG_FILES) {
	  char *pkg_version = pkg_version_str_alloc(pkg);
	  sprintf_alloc(&preinst_args, "install %s", pkg_version);
	  free(pkg_version);
     } else {
	  preinst_args = xstrdup("install");
     }

     err = pkg_run_script(pkg, "preinst", preinst_args);
     if (err) {
	  opkg_msg(ERROR, "Aborting installation of %s.\n", pkg->name);
	  return -1;
     }

     free(preinst_args);

     return 0;
}

static int
preinst_configure_unwind(pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does the following error unwind, should we?
	pkg->postrm abort-upgrade old-version
	OR pkg->postrm abort-install old-version
	OR pkg->postrm abort-install
     */
     return 0;
}

static char *
backup_filename_alloc(const char *file_name)
{
     char *backup;

     sprintf_alloc(&backup, "%s%s", file_name, OPKG_BACKUP_SUFFIX);

     return backup;
}


static int
backup_make_backup(const char *file_name)
{
     int err;
     char *backup;

     backup = backup_filename_alloc(file_name);
     err = file_copy(file_name, backup);
     if (err) {
	  opkg_msg(ERROR, "Failed to copy %s to %s\n",
		       file_name, backup);
     }

     free(backup);

     return err;
}

static int
backup_exists_for(const char *file_name)
{
     int ret;
     char *backup;

     backup = backup_filename_alloc(file_name);

     ret = file_exists(backup);

     free(backup);

     return ret;
}

static int
backup_remove(const char *file_name)
{
     char *backup;

     backup = backup_filename_alloc(file_name);
     unlink(backup);
     free(backup);

     return 0;
}

static int
backup_modified_conffiles(pkg_t *pkg, pkg_t *old_pkg)
{
     int err;
     conffile_list_elt_t *iter;
     conffile_t *cf;

     if (conf->noaction) return 0;

     /* Backup all modified conffiles */
     if (old_pkg) {
	  for (iter = nv_pair_list_first(&old_pkg->conffiles); iter; iter = nv_pair_list_next(&old_pkg->conffiles, iter)) {
	       char *cf_name;

	       cf = iter->data;
	       cf_name = root_filename_alloc(cf->name);

	       /* Don't worry if the conffile is just plain gone */
	       if (file_exists(cf_name) && conffile_has_been_modified(cf)) {
		    err = backup_make_backup(cf_name);
		    if (err) {
			 return err;
		    }
	       }
	       free(cf_name);
	  }
     }

     /* Backup all conffiles that were not conffiles in old_pkg */
     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  char *cf_name;
	  cf = (conffile_t *)iter->data;
	  cf_name = root_filename_alloc(cf->name);
	  /* Ignore if this was a conffile in old_pkg as well */
	  if (pkg_get_conffile(old_pkg, cf->name)) {
	       continue;
	  }

	  if (file_exists(cf_name) && (! backup_exists_for(cf_name))) {
	       err = backup_make_backup(cf_name);
	       if (err) {
		    return err;
	       }
	  }
	  free(cf_name);
     }

     return 0;
}

static int
backup_modified_conffiles_unwind(pkg_t *pkg, pkg_t *old_pkg)
{
     conffile_list_elt_t *iter;

     if (old_pkg) {
	  for (iter = nv_pair_list_first(&old_pkg->conffiles); iter; iter = nv_pair_list_next(&old_pkg->conffiles, iter)) {
	       backup_remove(((nv_pair_t *)iter->data)->name);
	  }
     }

     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  backup_remove(((nv_pair_t *)iter->data)->name);
     }

     return 0;
}


static int
check_data_file_clashes(pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	opkg takes a slightly different approach than dpkg at this
	point.  dpkg installs each file in the new package while
	creating a backup for any file that is replaced, (so that it
	can unwind if necessary).  To avoid complexity and redundant
	storage, opkg doesn't do any installation until later, (at the
	point at which dpkg removes the backups.

	But, we do have to check for data file clashes, since after
	installing a package with a file clash, removing either of the
	packages involved in the clash has the potential to break the
	other package.
     */
     str_list_t *files_list;
     str_list_elt_t *iter, *niter;
     char *filename;
     int clashes = 0;

     files_list = pkg_get_installed_files(pkg);
     if (files_list == NULL)
	     return -1;

     for (iter = str_list_first(files_list), niter = str_list_next(files_list, iter);
             iter;
             iter = niter, niter = str_list_next(files_list, iter)) {
	  filename = (char *) iter->data;
	  if (file_exists(filename) && (! file_is_dir(filename))) {
	       pkg_t *owner;
	       pkg_t *obs;

	       if (backup_exists_for(filename)) {
		    continue;
	       }

	       /* Pre-existing files are OK if force-overwrite was asserted. */
	       if (conf->force_overwrite) {
		    /* but we need to change who owns this file */
		    file_hash_set_file_owner(filename, pkg);
		    continue;
	       }

	       owner = file_hash_get_file_owner(filename);

	       /* Pre-existing files are OK if owned by the pkg being upgraded. */
	       if (owner && old_pkg) {
		    if (strcmp(owner->name, old_pkg->name) == 0) {
			 continue;
		    }
	       }

	       /* Pre-existing files are OK if owned by a package replaced by new pkg. */
	       if (owner) {
                    opkg_msg(DEBUG2, "Checking replaces for %s in package %s\n",
				filename, owner->name);
		    if (pkg_replaces(pkg, owner)) {
			 continue;
		    }
/* If the file that would be installed is owned by the same package, ( as per a reinstall or similar )
   then it's ok to overwrite. */
                    if (strcmp(owner->name,pkg->name)==0){
			 opkg_msg(INFO, "Replacing pre-existing file %s"
					" owned by package %s\n",
					filename, owner->name);
			 continue;
                    }
	       }

	       /* Pre-existing files are OK if they are obsolete */
	       obs = hash_table_get(&conf->obs_file_hash, filename);
	       if (obs) {
		    opkg_msg(INFO, "Pre-exiting file %s is obsolete."
				   " obs_pkg=%s\n",
				    filename, obs->name);
		    continue;
	       }

	       /* We have found a clash. */
	       opkg_msg(ERROR, "Package %s wants to install file %s\n"
			    "\tBut that file is already provided by package ",
			    pkg->name, filename);
	       if (owner) {
		    opkg_message(ERROR, "%s\n", owner->name);
	       } else {
		    opkg_message(ERROR, "<no package>\n"
			"Please move this file out of the way and try again.\n");
	       }
	       clashes++;
	  }
     }
     pkg_free_installed_files(pkg);

     return clashes;
}

/*
 * XXX: This function sucks, as does the below comment.
 */
static int
check_data_file_clashes_change(pkg_t *pkg, pkg_t *old_pkg)
{
    /* Basically that's the worst hack I could do to be able to change ownership of
       file list, but, being that we have no way to unwind the mods, due to structure
       of hash table, probably is the quickest hack too, whishing it would not slow-up thing too much.
       What we do here is change the ownership of file in hash if a replace ( or similar events
       happens )
       Only the action that are needed to change name should be considered.
       @@@ To change after 1.0 release.
     */
     str_list_t *files_list;
     str_list_elt_t *iter, *niter;

     char *root_filename = NULL;

     files_list = pkg_get_installed_files(pkg);
     if (files_list == NULL)
	     return -1;

     for (iter = str_list_first(files_list), niter = str_list_next(files_list, iter);
             iter;
             iter = niter, niter = str_list_next(files_list, niter)) {
	  char *filename = (char *) iter->data;
          if (root_filename) {
              free(root_filename);
              root_filename = NULL;
          }
	  root_filename = root_filename_alloc(filename);
	  if (file_exists(root_filename) && (! file_is_dir(root_filename))) {
	       pkg_t *owner;

	       owner = file_hash_get_file_owner(filename);

	       if (conf->force_overwrite) {
		    /* but we need to change who owns this file */
		    file_hash_set_file_owner(filename, pkg);
		    continue;
	       }


	       /* Pre-existing files are OK if owned by a package replaced by new pkg. */
	       if (owner) {
		    if (pkg_replaces(pkg, owner)) {
/* It's now time to change the owner of that file.
   It has been "replaced" from the new "Replaces", then I need to inform lists file about that.  */
			 opkg_msg(INFO, "Replacing pre-existing file %s "
					 "owned by package %s\n",
					 filename, owner->name);
		         file_hash_set_file_owner(filename, pkg);
			 continue;
		    }
	       }

	  }
     }
     if (root_filename) {
         free(root_filename);
         root_filename = NULL;
     }
     pkg_free_installed_files(pkg);

     return 0;
}

static int
check_data_file_clashes_unwind(pkg_t *pkg, pkg_t *old_pkg)
{
     /* Nothing to do since check_data_file_clashes doesn't change state */
     return 0;
}

static int
postrm_upgrade_old_pkg(pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY: dpkg does the following here, should we?
	1. If the package is being upgraded, call
	   old-postrm upgrade new-version
	2. If this fails, attempt:
	   new-postrm failed-upgrade old-version
	Error unwind, for both cases:
	   old-preinst abort-upgrade new-version    */
     return 0;
}

static int
postrm_upgrade_old_pkg_unwind(pkg_t *pkg, pkg_t *old_pkg)
{
     /* DPKG_INCOMPATIBILITY:
	dpkg does some things here that we don't do yet. Do we care?
	(See postrm_upgrade_old_pkg for details)
     */
    return 0;
}

static int
remove_obsolesced_files(pkg_t *pkg, pkg_t *old_pkg)
{
     int err = 0;
     str_list_t *old_files;
     str_list_elt_t *of;
     str_list_t *new_files;
     str_list_elt_t *nf;
     hash_table_t new_files_table;

     old_files = pkg_get_installed_files(old_pkg);
     if (old_files == NULL)
	  return -1;

     new_files = pkg_get_installed_files(pkg);
     if (new_files == NULL) {
          pkg_free_installed_files(old_pkg);
	  return -1;
     }

     new_files_table.entries = NULL;
     hash_table_init("new_files" , &new_files_table, 20);
     for (nf = str_list_first(new_files); nf; nf = str_list_next(new_files, nf)) {
         if (nf && nf->data)
            hash_table_insert(&new_files_table, nf->data, nf->data);
     }

     for (of = str_list_first(old_files); of; of = str_list_next(old_files, of)) {
	  pkg_t *owner;
	  char *old, *new;
	  old = (char *)of->data;
          new = (char *) hash_table_get (&new_files_table, old);
          if (new)
               continue;

	  if (file_is_dir(old)) {
	       continue;
	  }
	  owner = file_hash_get_file_owner(old);
	  if (owner != old_pkg) {
	       /* in case obsolete file no longer belongs to old_pkg */
	       continue;
	  }

	  /* old file is obsolete */
	  opkg_msg(NOTICE, "Removing obsolete file %s.\n", old);
	  if (!conf->noaction) {
	       err = unlink(old);
	       if (err) {
		    opkg_perror(ERROR, "unlinking %s failed", old);
	       }
	  }
     }

     hash_table_deinit(&new_files_table);
     pkg_free_installed_files(old_pkg);
     pkg_free_installed_files(pkg);

     return err;
}

static int
install_maintainer_scripts(pkg_t *pkg, pkg_t *old_pkg)
{
     int ret;
     char *prefix;

     sprintf_alloc(&prefix, "%s.", pkg->name);
     ret = pkg_extract_control_files_to_dir_with_prefix(pkg,
							pkg->dest->info_dir,
							prefix);
     free(prefix);
     return ret;
}

static int
remove_disappeared(pkg_t *pkg)
{
     /* DPKG_INCOMPATIBILITY:
	This is a fairly sophisticated dpkg operation. Shall we
	skip it? */

     /* Any packages all of whose files have been overwritten during the
	installation, and which aren't required for dependencies, are
	considered to have been removed. For each such package
	1. disappearer's-postrm disappear overwriter overwriter-version
	2. The package's maintainer scripts are removed
	3. It is noted in the status database as being in a sane state,
           namely not installed (any conffiles it may have are ignored,
	   rather than being removed by dpkg). Note that disappearing
	   packages do not have their prerm called, because dpkg doesn't
	   know in advance that the package is going to vanish.
     */
     return 0;
}

static int
install_data_files(pkg_t *pkg)
{
     int err;

     /* opkg takes a slightly different approach to data file backups
	than dpkg. Rather than removing backups at this point, we
	actually do the data file installation now. See comments in
	check_data_file_clashes() for more details. */

     opkg_msg(INFO, "Extracting data files to %s.\n", pkg->dest->root_dir);
     err = pkg_extract_data_files_to_dir(pkg, pkg->dest->root_dir);
     if (err) {
	  return err;
     }

     /* The "Essential" control field may only be present in the control
      * file and not in the Packages list. Ensure we capture it regardless.
      *
      * XXX: This should be fixed outside of opkg, in the Package list.
      */
     set_flags_from_control(pkg) ;

     opkg_msg(DEBUG, "Calling pkg_write_filelist.\n");
     err = pkg_write_filelist(pkg);
     if (err)
	  return err;

     /* XXX: FEATURE: opkg should identify any files which existed
	before installation and which were overwritten, (see
	check_data_file_clashes()). What it must do is remove any such
	files from the filelist of the old package which provided the
	file. Otherwise, if the old package were removed at some point
	it would break the new package. Removing the new package will
	also break the old one, but this cannot be helped since the old
	package's file has already been deleted. This is the importance
	of check_data_file_clashes(), and only allowing opkg to install
	a clashing package with a user force. */

     return 0;
}

static int
resolve_conffiles(pkg_t *pkg)
{
     conffile_list_elt_t *iter;
     conffile_t *cf;
     char *cf_backup;
     char *md5sum;

     if (conf->noaction) return 0;

     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  char *root_filename;
	  cf = (conffile_t *)iter->data;
	  root_filename = root_filename_alloc(cf->name);

	  /* Might need to initialize the md5sum for each conffile */
	  if (cf->value == NULL) {
	       cf->value = file_md5sum_alloc(root_filename);
	  }

	  if (!file_exists(root_filename)) {
	       free(root_filename);
	       continue;
	  }

	  cf_backup = backup_filename_alloc(root_filename);

          if (file_exists(cf_backup)) {
              /* Let's compute md5 to test if files are changed */
              md5sum = file_md5sum_alloc(cf_backup);
              if (md5sum && cf->value && strcmp(cf->value,md5sum) != 0 ) {
                  if (conf->force_maintainer) {
                      opkg_msg(NOTICE, "Conffile %s using maintainer's setting.\n",
				      cf_backup);
                  } else {
                      char *new_conffile;
                      sprintf_alloc(&new_conffile, "%s-opkg", root_filename);
                      opkg_msg(ERROR, "Existing conffile %s "
                           "is different from the conffile in the new package."
                           " The new conffile will be placed at %s.\n",
                           root_filename, new_conffile);
                      rename(root_filename, new_conffile);
                      rename(cf_backup, root_filename);
                      free(new_conffile);
		  }
              }
              unlink(cf_backup);
	      if (md5sum)
                  free(md5sum);
          }

	  free(cf_backup);
	  free(root_filename);
     }

     return 0;
}


int
opkg_install_by_name(const char *pkg_name)
{
     int cmp;
     pkg_t *old, *new;
     char *old_version, *new_version;

     old = pkg_hash_fetch_installed_by_name(pkg_name);
     if (old)
        opkg_msg(DEBUG2, "Old versions from pkg_hash_fetch %s.\n",
			old->version);

     new = pkg_hash_fetch_best_installation_candidate_by_name(pkg_name);
     if (new == NULL) {
	opkg_msg(NOTICE, "Unknown package '%s'.\n", pkg_name);
	return -1;
     }

     opkg_msg(DEBUG2, "Versions from pkg_hash_fetch:");
     if ( old )
        opkg_message(DEBUG2, " old %s ", old->version);
     opkg_message(DEBUG2, " new %s\n", new->version);

     new->state_flag |= SF_USER;
     if (old) {
	  old_version = pkg_version_str_alloc(old);
	  new_version = pkg_version_str_alloc(new);

	  cmp = pkg_compare_versions(old, new);
          if ( (conf->force_downgrade==1) && (cmp > 0) ){     /* We've been asked to allow downgrade  and version is precedent */
	     opkg_msg(DEBUG, "Forcing downgrade\n");
             cmp = -1 ;                                       /* then we force opkg to downgrade */
                                                              /* We need to use a value < 0 because in the 0 case we are asking to */
                                                              /* reinstall, and some check could fail asking the "force-reinstall" option */
          }
	  opkg_msg(DEBUG, "Comparing visible versions of pkg %s:"
		       "\n\t%s is installed "
		       "\n\t%s is available "
		       "\n\t%d was comparison result\n",
		       pkg_name, old_version, new_version, cmp);
	  if (cmp == 0) {
	       opkg_msg(NOTICE,
			    "Package %s (%s) installed in %s is up to date.\n",
			    old->name, old_version, old->dest->name);
	       free(old_version);
	       free(new_version);
	       return 0;
	  } else if (cmp > 0) {
	       opkg_msg(NOTICE,
			    "Not downgrading package %s on %s from %s to %s.\n",
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
     }

     opkg_msg(DEBUG2,"Calling opkg_install_pkg.\n");
     return opkg_install_pkg(new, 0);
}

/**
 *  @brief Really install a pkg_t
 */
int
opkg_install_pkg(pkg_t *pkg, int from_upgrade)
{
     int err = 0;
     int message = 0;
     pkg_t *old_pkg = NULL;
     pkg_vec_t *replacees;
     abstract_pkg_t *ab_pkg = NULL;
     int old_state_flag;
     char* file_md5;
#ifdef HAVE_SHA256
     char* file_sha256;
#endif
     sigset_t newset, oldset;

     if ( from_upgrade )
        message = 1;            /* Coming from an upgrade, and should change the output message */

     opkg_msg(DEBUG2, "Calling pkg_arch_supported.\n");

     if (!pkg_arch_supported(pkg)) {
	  opkg_msg(ERROR, "INTERNAL ERROR: architecture %s for pkg %s is unsupported.\n",
		       pkg->architecture, pkg->name);
	  return -1;
     }
     if (pkg->state_status == SS_INSTALLED && conf->nodeps == 0) {
	  err = satisfy_dependencies_for(pkg);
	  if (err)
		  return -1;

	  opkg_msg(NOTICE, "Package %s is already installed on %s.\n",
		       pkg->name, pkg->dest->name);
	  return 0;
     }

     if (pkg->dest == NULL) {
	  pkg->dest = conf->default_dest;
     }

     old_pkg = pkg_hash_fetch_installed_by_name(pkg->name);

     err = opkg_install_check_downgrade(pkg, old_pkg, message);
     if (err)
	     return -1;

     pkg->state_want = SW_INSTALL;
     if (old_pkg){
         old_pkg->state_want = SW_DEINSTALL; /* needed for check_data_file_clashes of dependencies */
     }

     err = check_conflicts_for(pkg);
     if (err)
	     return -1;

     /* this setup is to remove the upgrade scenario in the end when
	installing pkg A, A deps B & B deps on A. So both B and A are
	installed. Then A's installation is started resulting in an
	uncecessary upgrade */
     if (pkg->state_status == SS_INSTALLED)
	     return 0;

     err = verify_pkg_installable(pkg);
     if (err)
	     return -1;

     if (pkg->local_filename == NULL) {
         if(!conf->cache && conf->download_only){
             char cwd[4096];
             if(getcwd(cwd, sizeof(cwd)) != NULL)
                err = opkg_download_pkg(pkg, cwd);
             else
                return -1;
         } else {
             err = opkg_download_pkg(pkg, conf->tmp_dir);
         }
	  if (err) {
	       opkg_msg(ERROR, "Failed to download %s. "
			       "Perhaps you need to run 'opkg update'?\n",
			    pkg->name);
	       return -1;
	  }
     }

     /* check that the repository is valid */
     #if defined(HAVE_GPGME) || defined(HAVE_OPENSSL)
     char *list_file_name, *sig_file_name, *lists_dir;

     /* check to ensure the package has come from a repository */
     if (conf->check_signature && pkg->src)
     {
       sprintf_alloc (&lists_dir, "%s",
                     (conf->restrict_to_default_dest)
                      ? conf->default_dest->lists_dir
                      : conf->lists_dir);
       sprintf_alloc (&list_file_name, "%s/%s", lists_dir, pkg->src->name);
       sprintf_alloc (&sig_file_name, "%s/%s.sig", lists_dir, pkg->src->name);

       if (file_exists (sig_file_name))
       {
         if (opkg_verify_file (list_file_name, sig_file_name)){
           opkg_msg(ERROR, "Failed to verify the signature of %s.\n",
                           list_file_name);
           return -1;
         }
       }else{
         opkg_msg(ERROR, "Signature file is missing for %s. "
                         "Perhaps you need to run 'opkg update'?\n",
			 pkg->name);
         return -1;
       }

       free (lists_dir);
       free (list_file_name);
       free (sig_file_name);
     }
     #endif

     /* Check for md5 values */
     if (pkg->md5sum)
     {
         file_md5 = file_md5sum_alloc(pkg->local_filename);
         if (file_md5 && strcmp(file_md5, pkg->md5sum))
         {
              opkg_msg(ERROR, "Package %s md5sum mismatch. "
			"Either the opkg or the package index are corrupt. "
			"Try 'opkg update'.\n",
			pkg->name);
              free(file_md5);
              return -1;
         }
	 if (file_md5)
              free(file_md5);
     }

#ifdef HAVE_SHA256
     /* Check for sha256 value */
     if(pkg->sha256sum)
     {
         file_sha256 = file_sha256sum_alloc(pkg->local_filename);
         if (file_sha256 && strcmp(file_sha256, pkg->sha256sum))
         {
              opkg_msg(ERROR, "Package %s sha256sum mismatch. "
			"Either the opkg or the package index are corrupt. "
			"Try 'opkg update'.\n",
			pkg->name);
              free(file_sha256);
              return -1;
         }
	 if (file_sha256)
              free(file_sha256);
     }
#endif
     if(conf->download_only) {
         if (conf->nodeps == 0) {
             err = satisfy_dependencies_for(pkg);
             if (err)
                 return -1;
         }
         return 0;
     }

     if (pkg->tmp_unpack_dir == NULL) {
	  if (unpack_pkg_control_files(pkg) == -1) {
	       opkg_msg(ERROR, "Failed to unpack control files from %s.\n",
			       pkg->local_filename);
	       return -1;
	  }
     }

     err = update_file_ownership(pkg, old_pkg);
     if (err)
	     return -1;

     if (conf->nodeps == 0) {
	  err = satisfy_dependencies_for(pkg);
	  if (err)
		return -1;
          if (pkg->state_status == SS_UNPACKED)
               /* Circular dependency has installed it for us. */
		return 0;
     }

     replacees = pkg_vec_alloc();
     pkg_get_installed_replacees(pkg, replacees);

     /* this next section we do with SIGINT blocked to prevent inconsistency between opkg database and filesystem */

	  sigemptyset(&newset);
	  sigaddset(&newset, SIGINT);
	  sigprocmask(SIG_BLOCK, &newset, &oldset);

	  opkg_state_changed++;
	  pkg->state_flag |= SF_FILELIST_CHANGED;

	  if (old_pkg)
               pkg_remove_orphan_dependent(pkg, old_pkg);

	  /* XXX: BUG: we really should treat replacement more like an upgrade
	   *      Instead, we're going to remove the replacees
	   */
	  err = pkg_remove_installed_replacees(replacees);
	  if (err)
		  goto UNWIND_REMOVE_INSTALLED_REPLACEES;

	  err = prerm_upgrade_old_pkg(pkg, old_pkg);
	  if (err)
		  goto UNWIND_PRERM_UPGRADE_OLD_PKG;

	  err = prerm_deconfigure_conflictors(pkg, replacees);
	  if (err)
		  goto UNWIND_PRERM_DECONFIGURE_CONFLICTORS;

	  err = preinst_configure(pkg, old_pkg);
	  if (err)
		  goto UNWIND_PREINST_CONFIGURE;

	  err = backup_modified_conffiles(pkg, old_pkg);
	  if (err)
		  goto UNWIND_BACKUP_MODIFIED_CONFFILES;

	  err = check_data_file_clashes(pkg, old_pkg);
	  if (err)
		  goto UNWIND_CHECK_DATA_FILE_CLASHES;

	  err = postrm_upgrade_old_pkg(pkg, old_pkg);
	  if (err)
		  goto UNWIND_POSTRM_UPGRADE_OLD_PKG;

	  if (conf->noaction)
		  return 0;

	  /* point of no return: no unwinding after this */
	  if (old_pkg) {
	       old_pkg->state_want = SW_DEINSTALL;

	       if (old_pkg->state_flag & SF_NOPRUNE) {
		    opkg_msg(INFO, "Not removing obsolesced files because "
				    "package %s marked noprune.\n",
				    old_pkg->name);
	       } else {
		    opkg_msg(INFO, "Removing obsolesced files for %s\n",
				    old_pkg->name);
		    if (remove_obsolesced_files(pkg, old_pkg)) {
			opkg_msg(ERROR, "Failed to determine "
					"obsolete files from previously "
					"installed %s\n", old_pkg->name);
		    }
	       }

               /* removing files from old package, to avoid ghost files */
               remove_data_files_and_list(old_pkg);
               remove_maintainer_scripts(old_pkg);
	  }


	  opkg_msg(INFO, "Installing maintainer scripts.\n");
	  if (install_maintainer_scripts(pkg, old_pkg)) {
		opkg_msg(ERROR, "Failed to extract maintainer scripts for %s."
			       " Package debris may remain!\n",
			       pkg->name);
		goto pkg_is_hosed;
	  }

	  /* the following just returns 0 */
	  remove_disappeared(pkg);

	  opkg_msg(INFO, "Installing data files for %s.\n", pkg->name);

	  if (install_data_files(pkg)) {
		opkg_msg(ERROR, "Failed to extract data files for %s. "
				"Package debris may remain!\n",
			       pkg->name);
		goto pkg_is_hosed;
	  }

	  err = check_data_file_clashes_change(pkg, old_pkg);
	  if (err) {
		opkg_msg(ERROR, "check_data_file_clashes_change() failed for "
			       "for files belonging to %s.\n",
			       pkg->name);
	  }

	  opkg_msg(INFO, "Resolving conf files for %s\n", pkg->name);
	  resolve_conffiles(pkg);

	  pkg->state_status = SS_UNPACKED;
	  old_state_flag = pkg->state_flag;
	  pkg->state_flag &= ~SF_PREFER;
	  opkg_msg(DEBUG, "pkg=%s old_state_flag=%x state_flag=%x\n",
			  pkg->name, old_state_flag, pkg->state_flag);

	  if (old_pkg)
	       old_pkg->state_status = SS_NOT_INSTALLED;

	  time(&pkg->installed_time);

	  ab_pkg = pkg->parent;
	  if (ab_pkg)
	       ab_pkg->state_status = pkg->state_status;

	  sigprocmask(SIG_UNBLOCK, &newset, &oldset);
          pkg_vec_free (replacees);
	  return 0;


     UNWIND_POSTRM_UPGRADE_OLD_PKG:
	  postrm_upgrade_old_pkg_unwind(pkg, old_pkg);
     UNWIND_CHECK_DATA_FILE_CLASHES:
	  check_data_file_clashes_unwind(pkg, old_pkg);
     UNWIND_BACKUP_MODIFIED_CONFFILES:
	  backup_modified_conffiles_unwind(pkg, old_pkg);
     UNWIND_PREINST_CONFIGURE:
	  preinst_configure_unwind(pkg, old_pkg);
     UNWIND_PRERM_DECONFIGURE_CONFLICTORS:
	  prerm_deconfigure_conflictors_unwind(pkg, replacees);
     UNWIND_PRERM_UPGRADE_OLD_PKG:
	  prerm_upgrade_old_pkg_unwind(pkg, old_pkg);
     UNWIND_REMOVE_INSTALLED_REPLACEES:
	  pkg_remove_installed_replacees_unwind(replacees);

pkg_is_hosed:
	  sigprocmask(SIG_UNBLOCK, &newset, &oldset);

          pkg_vec_free (replacees);
	  return -1;
}
