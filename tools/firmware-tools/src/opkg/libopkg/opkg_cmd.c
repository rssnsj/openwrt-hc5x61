/* opkg_cmd.c - the opkg package management system

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
#include <dirent.h>
#include <glob.h>
#include <fnmatch.h>
#include <signal.h>
#include <unistd.h>

#include "opkg_conf.h"
#include "opkg_cmd.h"
#include "opkg_message.h"
#include "release.h"
#include "pkg.h"
#include "pkg_dest.h"
#include "pkg_parse.h"
#include "sprintf_alloc.h"
#include "pkg.h"
#include "file_util.h"
#include "libbb/libbb.h"
#include "opkg_utils.h"
#include "opkg_defines.h"
#include "opkg_download.h"
#include "opkg_install.h"
#include "opkg_upgrade.h"
#include "opkg_remove.h"
#include "opkg_configure.h"
#include "xsystem.h"

static void
print_pkg(pkg_t *pkg)
{
	char *version = pkg_version_str_alloc(pkg);
	if (pkg->description)
		printf("%s - %s - %s\n", pkg->name, version, pkg->description);
	else
		printf("%s - %s\n", pkg->name, version);
	free(version);
}

int opkg_state_changed;

static void
write_status_files_if_changed(void)
{
     if (opkg_state_changed && !conf->noaction) {
	  opkg_msg(INFO, "Writing status file.\n");
	  opkg_conf_write_status_files();
	  pkg_write_changed_filelists();
     } else {
	  opkg_msg(DEBUG, "Nothing to be done.\n");
     }
}

static void
sigint_handler(int sig)
{
     signal(sig, SIG_DFL);
     opkg_msg(NOTICE, "Interrupted. Writing out status database.\n");
     write_status_files_if_changed();
     exit(128 + sig);
}

static int
opkg_update_cmd(int argc, char **argv)
{
     char *tmp;
     int err;
     int failures;
     char *lists_dir;
     pkg_src_list_elt_t *iter;
     pkg_src_t *src;


    sprintf_alloc(&lists_dir, "%s", conf->restrict_to_default_dest ? conf->default_dest->lists_dir : conf->lists_dir);

    if (! file_is_dir(lists_dir)) {
	  if (file_exists(lists_dir)) {
	       opkg_msg(ERROR, "%s exists, but is not a directory.\n",
			    lists_dir);
	       free(lists_dir);
	       return -1;
	  }
	  err = file_mkdir_hier(lists_dir, 0755);
	  if (err) {
	       free(lists_dir);
	       return -1;
	  }
     }

     failures = 0;

     sprintf_alloc(&tmp, "%s/update-XXXXXX", conf->tmp_dir);
     if (mkdtemp (tmp) == NULL) {
	 opkg_perror(ERROR, "Failed to make temp dir %s", conf->tmp_dir);
	 return -1;
     }


     for (iter = void_list_first(&conf->dist_src_list); iter; iter = void_list_next(&conf->dist_src_list, iter)) {
	  char *url, *list_file_name;

	  src = (pkg_src_t *)iter->data;

	  sprintf_alloc(&url, "%s/dists/%s/Release", src->value, src->name);

	  sprintf_alloc(&list_file_name, "%s/%s", lists_dir, src->name);
	  err = opkg_download(url, list_file_name, NULL, NULL, 0);
	  if (!err) {
	       opkg_msg(NOTICE, "Downloaded release files for dist %s.\n",
			    src->name);
	       release_t *release = release_new(); 
	       err = release_init_from_file(release, list_file_name);
	       if (!err) {
		    if (!release_comps_supported(release, src->extra_data))
			 err = -1;
	       }
	       if (!err) {
		    err = release_download(release, src, lists_dir, tmp);
	       }
	       release_deinit(release); 
	       if (err)
		    unlink(list_file_name);
	  }

	  if (err)
	       failures++;

	  free(list_file_name);
	  free(url);
     }

     for (iter = void_list_first(&conf->pkg_src_list); iter; iter = void_list_next(&conf->pkg_src_list, iter)) {
	  char *url, *list_file_name;

	  src = (pkg_src_t *)iter->data;

	  if (src->extra_data && strcmp(src->extra_data, "__dummy__ "))
	      continue;

	  if (src->extra_data)	/* debian style? */
	      sprintf_alloc(&url, "%s/%s/%s", src->value, src->extra_data,
			    src->gzip ? "Packages.gz" : "Packages");
	  else
	      sprintf_alloc(&url, "%s/%s", src->value, src->gzip ? "Packages.gz" : "Packages");

	  sprintf_alloc(&list_file_name, "%s/%s", lists_dir, src->name);
	  if (src->gzip) {
	      char *tmp_file_name;
	      FILE *in, *out;

	      sprintf_alloc (&tmp_file_name, "%s/%s.gz", tmp, src->name);
	      err = opkg_download(url, tmp_file_name, NULL, NULL, 0);
	      if (err == 0) {
		   opkg_msg(NOTICE, "Inflating %s.\n", url);
		   in = fopen (tmp_file_name, "r");
		   out = fopen (list_file_name, "w");
		   if (in && out)
			unzip (in, out);
		   else
			err = 1;
		   if (in)
			fclose (in);
		   if (out)
			fclose (out);
		   unlink (tmp_file_name);
	      }
	      free(tmp_file_name);
	  } else
	      err = opkg_download(url, list_file_name, NULL, NULL, 0);
	  if (err) {
	       failures++;
	  } else {
	       opkg_msg(NOTICE, "Updated list of available packages in %s.\n",
			    list_file_name);
	  }
	  free(url);
#if defined(HAVE_GPGME) || defined(HAVE_OPENSSL)
          if (conf->check_signature) {
              /* download detached signitures to verify the package lists */
              /* get the url for the sig file */
              if (src->extra_data)	/* debian style? */
                  sprintf_alloc(&url, "%s/%s/%s", src->value, src->extra_data,
                          "Packages.sig");
              else
                  sprintf_alloc(&url, "%s/%s", src->value, "Packages.sig");

              /* create temporary file for it */
              char *tmp_file_name;

              /* Put the signature in the right place */
              sprintf_alloc (&tmp_file_name, "%s/%s.sig", lists_dir, src->name);

              err = opkg_download(url, tmp_file_name, NULL, NULL, 0);
              if (err) {
                  failures++;
                  opkg_msg(NOTICE, "Signature check failed.\n");
              } else {
                  err = opkg_verify_file (list_file_name, tmp_file_name);
                  if (err == 0)
                      opkg_msg(NOTICE, "Signature check passed.\n");
                  else
                      opkg_msg(NOTICE, "Signature check failed.\n");
              }
              if (err) {
                  /* The signature was wrong so delete it */
                  opkg_msg(NOTICE, "Remove wrong Signature file.\n");
                  unlink (tmp_file_name);
                  unlink (list_file_name);
              }
              /* We shouldn't unlink the signature ! */
              // unlink (tmp_file_name);
              free (tmp_file_name);
              free (url);
          }
#else
          // Do nothing
#endif
	  free(list_file_name);
     }
     rmdir (tmp);
     free (tmp);
     free(lists_dir);

     return failures;
}


struct opkg_intercept
{
    char *oldpath;
    char *statedir;
};

typedef struct opkg_intercept *opkg_intercept_t;

static opkg_intercept_t
opkg_prep_intercepts(void)
{
    opkg_intercept_t ctx;
    char *newpath;

    ctx = xcalloc(1, sizeof (*ctx));
    ctx->oldpath = xstrdup(getenv("PATH"));
    sprintf_alloc(&newpath, "%s/opkg/intercept:%s", DATADIR, ctx->oldpath);
    sprintf_alloc(&ctx->statedir, "%s/opkg-intercept-XXXXXX", conf->tmp_dir);

    if (mkdtemp(ctx->statedir) == NULL) {
        opkg_perror(ERROR,"Failed to make temp dir %s", ctx->statedir);
	free(ctx->oldpath);
	free(ctx->statedir);
        free(newpath);
	free(ctx);
	return NULL;
    }

    setenv("OPKG_INTERCEPT_DIR", ctx->statedir, 1);
    setenv("PATH", newpath, 1);
    free(newpath);

    return ctx;
}

static int
opkg_finalize_intercepts(opkg_intercept_t ctx)
{
    DIR *dir;
    int err = 0;

    setenv ("PATH", ctx->oldpath, 1);
    free (ctx->oldpath);

    dir = opendir (ctx->statedir);
    if (dir) {
	struct dirent *de;
	while (de = readdir (dir), de != NULL) {
	    char *path;

	    if (de->d_name[0] == '.')
		continue;

	    sprintf_alloc (&path, "%s/%s", ctx->statedir, de->d_name);
	    if (access (path, X_OK) == 0) {
		const char *argv[] = {"sh", "-c", path, NULL};
		xsystem (argv);
	    }
	    free (path);
	}
        closedir(dir);
    } else
	opkg_perror(ERROR, "Failed to open dir %s", ctx->statedir);

    rm_r(ctx->statedir);
    free (ctx->statedir);
    free (ctx);

    return err;
}

/* For package pkg do the following: If it is already visited, return. If not,
   add it in visited list and recurse to its deps. Finally, add it to ordered
   list.
   pkg_vec all contains all available packages in repos.
   pkg_vec visited contains packages already visited by this function, and is
   used to end recursion and avoid an infinite loop on graph cycles.
   pkg_vec ordered will finally contain the ordered set of packages.
*/
static int
opkg_recurse_pkgs_in_order(pkg_t *pkg, pkg_vec_t *all,
                               pkg_vec_t *visited, pkg_vec_t *ordered)
{
    int j,k,l,m;
    int count;
    pkg_t *dep;
    compound_depend_t * compound_depend;
    depend_t ** possible_satisfiers;
    abstract_pkg_t *abpkg;
    abstract_pkg_t **dependents;

    /* If it's just an available package, that is, not installed and not even
       unpacked, skip it */
    /* XXX: This is probably an overkill, since a state_status != SS_UNPACKED
       would do here. However, if there is an intermediate node (pkg) that is
       configured and installed between two unpacked packages, the latter
       won't be properly reordered, unless all installed/unpacked pkgs are
       checked */
    if (pkg->state_status == SS_NOT_INSTALLED)
        return 0;

    /* If the  package has already been visited (by this function), skip it */
    for(j = 0; j < visited->len; j++)
        if ( ! strcmp(visited->pkgs[j]->name, pkg->name)) {
            opkg_msg(DEBUG, "pkg %s already visited, skipping.\n", pkg->name);
            return 0;
        }

    pkg_vec_insert(visited, pkg);

    count = pkg->pre_depends_count + pkg->depends_count + \
        pkg->recommends_count + pkg->suggests_count;

    opkg_msg(DEBUG, "pkg %s.\n", pkg->name);

    /* Iterate over all the dependencies of pkg. For each one, find a package
       that is either installed or unpacked and satisfies this dependency.
       (there should only be one such package per dependency installed or
       unpacked). Then recurse to the dependency package */
    for (j=0; j < count ; j++) {
        compound_depend = &pkg->depends[j];
        possible_satisfiers = compound_depend->possibilities;
        for (k=0; k < compound_depend->possibility_count ; k++) {
            abpkg = possible_satisfiers[k]->pkg;
            dependents = abpkg->provided_by->pkgs;
            l = 0;
            if (dependents != NULL)
                while (l < abpkg->provided_by->len && dependents[l] != NULL) {
                    opkg_msg(DEBUG, "Descending on pkg %s.\n",
                                 dependents [l]->name);

                    /* find whether dependent l is installed or unpacked,
                     * and then find which package in the list satisfies it */
                    for(m = 0; m < all->len; m++) {
                        dep = all->pkgs[m];
                        if ( dep->state_status != SS_NOT_INSTALLED)
                            if ( ! strcmp(dep->name, dependents[l]->name)) {
                                opkg_recurse_pkgs_in_order(dep, all,
                                                           visited, ordered);
                                /* Stop the outer loop */
                                l = abpkg->provided_by->len;
                                /* break from the inner loop */
                                break;
                            }
                    }
                    l++;
                }
        }
    }

    /* When all recursions from this node down, are over, and all
       dependencies have been added in proper order in the ordered array, add
       also the package pkg to ordered array */
    pkg_vec_insert(ordered, pkg);

    return 0;

}

static int
opkg_configure_packages(char *pkg_name)
{
     pkg_vec_t *all, *ordered, *visited;
     int i;
     pkg_t *pkg;
     opkg_intercept_t ic;
     int r, err = 0;

     opkg_msg(INFO, "Configuring unpacked packages.\n");

     all = pkg_vec_alloc();

     pkg_hash_fetch_available(all);

     /* Reorder pkgs in order to be configured according to the Depends: tag
        order */
     opkg_msg(INFO, "Reordering packages before configuring them...\n");
     ordered = pkg_vec_alloc();
     visited = pkg_vec_alloc();
     for(i = 0; i < all->len; i++) {
         pkg = all->pkgs[i];
         opkg_recurse_pkgs_in_order(pkg, all, visited, ordered);
     }

     ic = opkg_prep_intercepts();
     if (ic == NULL) {
	     err = -1;
	     goto error;
     }

     for(i = 0; i < ordered->len; i++) {
	  pkg = ordered->pkgs[i];

	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
	       continue;

	  if (pkg->state_status == SS_UNPACKED) {
	       opkg_msg(NOTICE, "Configuring %s.\n", pkg->name);
	       r = opkg_configure(pkg);
	       if (r == 0) {
		    pkg->state_status = SS_INSTALLED;
		    pkg->parent->state_status = SS_INSTALLED;
		    pkg->state_flag &= ~SF_PREFER;
		    opkg_state_changed++;
	       } else {
		    err = -1;
	       }
	  }
     }

     if (opkg_finalize_intercepts (ic))
	 err = -1;

error:
     pkg_vec_free(all);
     pkg_vec_free(ordered);
     pkg_vec_free(visited);

     return err;
}

static int
opkg_remove_cmd(int argc, char **argv);

static int
opkg_install_cmd(int argc, char **argv)
{
     int i;
     char *arg;
     int err = 0;

     if (conf->force_reinstall) {
	     int saved_force_depends = conf->force_depends;
	     conf->force_depends = 1;
	     (void)opkg_remove_cmd(argc, argv);
	     conf->force_depends = saved_force_depends;
	     conf->force_reinstall = 0;
     }

     signal(SIGINT, sigint_handler);

     /*
      * Now scan through package names and install
      */
     for (i=0; i < argc; i++) {
	  arg = argv[i];

          opkg_msg(DEBUG2, "%s\n", arg);
          if (opkg_prepare_url_for_install(arg, &argv[i]))
              return -1;
     }
     pkg_info_preinstall_check();

     for (i=0; i < argc; i++) {
	  arg = argv[i];
          if (opkg_install_by_name(arg)) {
	       opkg_msg(ERROR, "Cannot install package %s.\n", arg);
	       err = -1;
	  }
     }

     if (opkg_configure_packages(NULL))
	  err = -1;

     write_status_files_if_changed();

     return err;
}

static int
opkg_upgrade_cmd(int argc, char **argv)
{
     int i;
     pkg_t *pkg;
     int err = 0;

     signal(SIGINT, sigint_handler);

     if (argc) {
	  for (i=0; i < argc; i++) {
	       char *arg = argv[i];

               if (opkg_prepare_url_for_install(arg, &arg))
                   return -1;
	  }
	  pkg_info_preinstall_check();

	  for (i=0; i < argc; i++) {
	       char *arg = argv[i];
	       if (conf->restrict_to_default_dest) {
		    pkg = pkg_hash_fetch_installed_by_name_dest(argv[i],
							conf->default_dest);
		    if (pkg == NULL) {
			 opkg_msg(NOTICE, "Package %s not installed in %s.\n",
				      argv[i], conf->default_dest->name);
			 continue;
		    }
	       } else {
		    pkg = pkg_hash_fetch_installed_by_name(argv[i]);
	       }
	       if (pkg) {
		    if (opkg_upgrade_pkg(pkg))
			    err = -1;
	       } else {
		    if (opkg_install_by_name(arg))
			    err = -1;
               }
	  }
     }

     if (opkg_configure_packages(NULL))
	  err = -1;

     write_status_files_if_changed();

     return err;
}

static int
opkg_download_cmd(int argc, char **argv)
{
     int i, err = 0;
     char *arg;
     pkg_t *pkg;

     pkg_info_preinstall_check();
     for (i = 0; i < argc; i++) {
	  arg = argv[i];

	  pkg = pkg_hash_fetch_best_installation_candidate_by_name(arg);
	  if (pkg == NULL) {
	       opkg_msg(ERROR, "Cannot find package %s.\n", arg);
	       continue;
	  }

	  if (opkg_download_pkg(pkg, "."))
		  err = -1;

	  if (err) {
	       opkg_msg(ERROR, "Failed to download %s.\n", pkg->name);
	  } else {
	       opkg_msg(NOTICE, "Downloaded %s as %s.\n",
			    pkg->name, pkg->local_filename);
	  }
     }

     return err;
}


static int
opkg_list_cmd(int argc, char **argv)
{
     int i;
     pkg_vec_t *available;
     pkg_t *pkg;
     char *pkg_name = NULL;

     if (argc > 0) {
	  pkg_name = argv[0];
     }
     available = pkg_vec_alloc();
     pkg_hash_fetch_available(available);
     pkg_vec_sort(available, pkg_compare_names);
     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  /* if we have package name or pattern and pkg does not match, then skip it */
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
	       continue;
          print_pkg(pkg);
     }
     pkg_vec_free(available);

     return 0;
}


static int
opkg_list_installed_cmd(int argc, char **argv)
{
     int i ;
     pkg_vec_t *available;
     pkg_t *pkg;
     char *pkg_name = NULL;

     if (argc > 0) {
	  pkg_name = argv[0];
     }
     available = pkg_vec_alloc();
     pkg_hash_fetch_all_installed(available);
     pkg_vec_sort(available, pkg_compare_names);
     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  /* if we have package name or pattern and pkg does not match, then skip it */
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
	       continue;
          print_pkg(pkg);
     }

     pkg_vec_free(available);

     return 0;
}

static int
opkg_list_changed_conffiles_cmd(int argc, char **argv)
{
     int i ;
     pkg_vec_t *available;
     pkg_t *pkg;
     char *pkg_name = NULL;
     conffile_list_elt_t *iter;
     conffile_t *cf;

     if (argc > 0) {
	  pkg_name = argv[0];
     }
     available = pkg_vec_alloc();
     pkg_hash_fetch_all_installed(available);
     pkg_vec_sort(available, pkg_compare_names);
     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  /* if we have package name or pattern and pkg does not match, then skip it */
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
	    continue;
	  if (nv_pair_list_empty(&pkg->conffiles))
	    continue;
	  for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	    cf = (conffile_t *)iter->data;
	    if (cf->name && cf->value && conffile_has_been_modified(cf))
	      printf("%s\n", cf->name);
	  }
     }
     pkg_vec_free(available);
     return 0;
}

static int
opkg_list_upgradable_cmd(int argc, char **argv)
{
    struct active_list *head = prepare_upgrade_list();
    struct active_list *node=NULL;
    pkg_t *_old_pkg, *_new_pkg;
    char *old_v, *new_v;
    for (node = active_list_next(head, head); node;node = active_list_next(head,node)) {
        _old_pkg = list_entry(node, pkg_t, list);
        _new_pkg = pkg_hash_fetch_best_installation_candidate_by_name(_old_pkg->name);
	if (_new_pkg == NULL)
		continue;
        old_v = pkg_version_str_alloc(_old_pkg);
        new_v = pkg_version_str_alloc(_new_pkg);
        printf("%s - %s - %s\n", _old_pkg->name, old_v, new_v);
        free(old_v);
        free(new_v);
    }
    active_list_head_delete(head);
    return 0;
}

static int
opkg_info_status_cmd(int argc, char **argv, int installed_only)
{
     int i;
     pkg_vec_t *available;
     pkg_t *pkg;
     char *pkg_name = NULL;

     if (argc > 0) {
	  pkg_name = argv[0];
     }

     available = pkg_vec_alloc();
     if (installed_only)
	  pkg_hash_fetch_all_installed(available);
     else
	  pkg_hash_fetch_available(available);

     for (i=0; i < available->len; i++) {
	  pkg = available->pkgs[i];
	  if (pkg_name && fnmatch(pkg_name, pkg->name, 0)) {
	       continue;
	  }

	  pkg_formatted_info(stdout, pkg);

	  if (conf->verbosity >= NOTICE) {
	       conffile_list_elt_t *iter;
	       for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
		    conffile_t *cf = (conffile_t *)iter->data;
		    int modified = conffile_has_been_modified(cf);
		    if (cf->value)
		        opkg_msg(INFO, "conffile=%s md5sum=%s modified=%d.\n",
				 cf->name, cf->value, modified);
	       }
	  }
     }
     pkg_vec_free(available);

     return 0;
}

static int
opkg_info_cmd(int argc, char **argv)
{
     return opkg_info_status_cmd(argc, argv, 0);
}

static int
opkg_status_cmd(int argc, char **argv)
{
     return opkg_info_status_cmd(argc, argv, 1);
}

static int
opkg_configure_cmd(int argc, char **argv)
{
	int err;
	char *pkg_name = NULL;

	if (argc > 0)
		pkg_name = argv[0];

	err = opkg_configure_packages(pkg_name);

	write_status_files_if_changed();

	return err;
}

static int
opkg_remove_cmd(int argc, char **argv)
{
     int i, a, done, err = 0;
     pkg_t *pkg;
     pkg_t *pkg_to_remove;
     pkg_vec_t *available;

     done = 0;

     signal(SIGINT, sigint_handler);

     pkg_info_preinstall_check();

     available = pkg_vec_alloc();
     pkg_hash_fetch_all_installed(available);

     for (i=0; i<argc; i++) {
        for (a=0; a<available->len; a++) {
            pkg = available->pkgs[a];
	    if (fnmatch(argv[i], pkg->name, 0)) {
               continue;
            }
            if (conf->restrict_to_default_dest) {
	         pkg_to_remove = pkg_hash_fetch_installed_by_name_dest(
				        pkg->name,
				        conf->default_dest);
            } else {
	         pkg_to_remove = pkg_hash_fetch_installed_by_name(pkg->name);
            }

            if (pkg_to_remove == NULL) {
	         opkg_msg(ERROR, "Package %s is not installed.\n", pkg->name);
	         continue;
            }
            if (pkg->state_status == SS_NOT_INSTALLED) {
	         opkg_msg(ERROR, "Package %s not installed.\n", pkg->name);
                 continue;
            }

            if (opkg_remove_pkg(pkg_to_remove, 0))
	         err = -1;
	    else
                 done = 1;
        }
     }

     pkg_vec_free(available);

     if (done == 0)
        opkg_msg(NOTICE, "No packages removed.\n");

     write_status_files_if_changed();
     return err;
}

static int
opkg_flag_cmd(int argc, char **argv)
{
     int i;
     pkg_t *pkg;
     const char *flags = argv[0];

     signal(SIGINT, sigint_handler);

     for (i=1; i < argc; i++) {
	  if (conf->restrict_to_default_dest) {
	       pkg = pkg_hash_fetch_installed_by_name_dest(argv[i],
							   conf->default_dest);
	  } else {
	       pkg = pkg_hash_fetch_installed_by_name(argv[i]);
	  }

	  if (pkg == NULL) {
	       opkg_msg(ERROR, "Package %s is not installed.\n", argv[i]);
	       continue;
	  }
          if (( strcmp(flags,"hold")==0)||( strcmp(flags,"noprune")==0)||
              ( strcmp(flags,"user")==0)||( strcmp(flags,"ok")==0)) {
	      pkg->state_flag = pkg_state_flag_from_str(flags);
          }

	  /*
	   * Useful if a package is installed in an offline_root, and
	   * should be configured by opkg-cl configure at a later date.
	   */
          if (( strcmp(flags,"installed")==0)||( strcmp(flags,"unpacked")==0)){
	      pkg->state_status = pkg_state_status_from_str(flags);
          }

	  opkg_state_changed++;
	  opkg_msg(NOTICE, "Setting flags for package %s to %s.\n",
		       pkg->name, flags);
     }

     write_status_files_if_changed();
     return 0;
}

static int
opkg_files_cmd(int argc, char **argv)
{
     pkg_t *pkg;
     str_list_t *files;
     str_list_elt_t *iter;
     char *pkg_version;

     if (argc < 1) {
	  return -1;
     }

     pkg = pkg_hash_fetch_installed_by_name(argv[0]);
     if (pkg == NULL) {
	  opkg_msg(ERROR, "Package %s not installed.\n", argv[0]);
	  return 0;
     }

     files = pkg_get_installed_files(pkg);
     pkg_version = pkg_version_str_alloc(pkg);

     printf("Package %s (%s) is installed on %s and has the following files:\n",
		pkg->name, pkg_version, pkg->dest->name);

     for (iter=str_list_first(files); iter; iter=str_list_next(files, iter))
          printf("%s\n", (char *)iter->data);

     free(pkg_version);
     pkg_free_installed_files(pkg);

     return 0;
}

static int
opkg_depends_cmd(int argc, char **argv)
{
	int i, j, k;
	int depends_count;
	pkg_vec_t *available_pkgs;
	compound_depend_t *cdep;
	pkg_t *pkg;
	char *str;

	pkg_info_preinstall_check();

	available_pkgs = pkg_vec_alloc();
	if (conf->query_all)
	       pkg_hash_fetch_available(available_pkgs);
	else
	       pkg_hash_fetch_all_installed(available_pkgs);

	for (i=0; i<argc; i++) {
		for (j=0; j<available_pkgs->len; j++) {
			pkg = available_pkgs->pkgs[j];

			if (fnmatch(argv[i], pkg->name, 0) != 0)
				continue;

			depends_count = pkg->depends_count +
					pkg->pre_depends_count +
					pkg->recommends_count +
					pkg->suggests_count;

			opkg_msg(NOTICE, "%s depends on:\n", pkg->name);

			for (k=0; k<depends_count; k++) {
				cdep = &pkg->depends[k];

				if (cdep->type != DEPEND)
				      continue;

				str = pkg_depend_str(pkg, k);
				opkg_msg(NOTICE, "\t%s\n", str);
				free(str);
			}

	       }
	}

	pkg_vec_free(available_pkgs);
	return 0;
}

static int
pkg_mark_provides(pkg_t *pkg)
{
     int provides_count = pkg->provides_count;
     abstract_pkg_t **provides = pkg->provides;
     int i;
     pkg->parent->state_flag |= SF_MARKED;
     for (i = 0; i < provides_count; i++) {
	  provides[i]->state_flag |= SF_MARKED;
     }
     return 0;
}

enum what_field_type {
  WHATDEPENDS,
  WHATCONFLICTS,
  WHATPROVIDES,
  WHATREPLACES,
  WHATRECOMMENDS,
  WHATSUGGESTS
};

static int
opkg_what_depends_conflicts_cmd(enum depend_type what_field_type, int recursive, int argc, char **argv)
{
	depend_t *possibility;
	compound_depend_t *cdep;
	pkg_vec_t *available_pkgs;
	pkg_t *pkg;
	int i, j, k, l;
	int changed, count;
	const char *rel_str = NULL;
	char *ver;

	switch (what_field_type) {
	case DEPEND: rel_str = "depends on"; break;
	case CONFLICTS: rel_str = "conflicts with"; break;
	case SUGGEST: rel_str = "suggests"; break;
	case RECOMMEND: rel_str = "recommends"; break;
	default: return -1;
	}

	available_pkgs = pkg_vec_alloc();

	if (conf->query_all)
	       pkg_hash_fetch_available(available_pkgs);
	else
	       pkg_hash_fetch_all_installed(available_pkgs);

	/* mark the root set */
	pkg_vec_clear_marks(available_pkgs);
	opkg_msg(NOTICE, "Root set:\n");
	for (i = 0; i < argc; i++)
	       pkg_vec_mark_if_matches(available_pkgs, argv[i]);

	for (i = 0; i < available_pkgs->len; i++) {
	       pkg = available_pkgs->pkgs[i];
	       if (pkg->state_flag & SF_MARKED) {
		    /* mark the parent (abstract) package */
		    pkg_mark_provides(pkg);
		    opkg_msg(NOTICE, "  %s\n", pkg->name);
	       }
	}

	opkg_msg(NOTICE, "What %s root set\n", rel_str);
	do {
		changed = 0;

		for (j=0; j<available_pkgs->len; j++) {

			pkg = available_pkgs->pkgs[j];
			count = ((what_field_type == CONFLICTS)
				 ? pkg->conflicts_count
				 : pkg->pre_depends_count +
				 pkg->depends_count +
				 pkg->recommends_count +
				 pkg->suggests_count);

			/* skip this package if it is already marked */
			if (pkg->parent->state_flag & SF_MARKED)
				continue;

			for (k=0; k<count; k++) {
				cdep = (what_field_type == CONFLICTS)
					? &pkg->conflicts[k]
					: &pkg->depends[k];

				if (what_field_type != cdep->type)
					continue;

				for (l=0; l<cdep->possibility_count; l++) {
					possibility = cdep->possibilities[l];

					if ((possibility->pkg->state_flag
								& SF_MARKED)
							!= SF_MARKED)
						continue;

					/* mark the depending package so we
					* won't visit it again */
					pkg->state_flag |= SF_MARKED;
					pkg_mark_provides(pkg);
					changed++;

					ver = pkg_version_str_alloc(pkg);
				        opkg_msg(NOTICE, "\t%s %s\t%s %s",
							pkg->name,
							ver,
							rel_str,
							possibility->pkg->name);
					free(ver);
					if (possibility->version) {
						opkg_msg(NOTICE, " (%s%s)",
							constraint_to_str(possibility->constraint),
							possibility->version);
					}
					if (!pkg_dependence_satisfiable(possibility))
						opkg_msg(NOTICE,
							" unsatisfiable");
					opkg_message(NOTICE, "\n");
					goto next_package;
				}
			}
next_package:
			;
		}
	} while (changed && recursive);

	pkg_vec_free(available_pkgs);

	return 0;
}

static int
opkg_whatdepends_recursively_cmd(int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(DEPEND, 1, argc, argv);
}

static int
opkg_whatdepends_cmd(int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(DEPEND, 0, argc, argv);
}

static int
opkg_whatsuggests_cmd(int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(SUGGEST, 0, argc, argv);
}

static int
opkg_whatrecommends_cmd(int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(RECOMMEND, 0, argc, argv);
}

static int
opkg_whatconflicts_cmd(int argc, char **argv)
{
     return opkg_what_depends_conflicts_cmd(CONFLICTS, 0, argc, argv);
}

static int
opkg_what_provides_replaces_cmd(enum what_field_type what_field_type, int argc, char **argv)
{

     if (argc > 0) {
	  pkg_vec_t *available_pkgs = pkg_vec_alloc();
	  const char *rel_str = (what_field_type == WHATPROVIDES ? "provides" : "replaces");
	  int i;

	  pkg_info_preinstall_check();

	  if (conf->query_all)
	       pkg_hash_fetch_available(available_pkgs);
	  else
	       pkg_hash_fetch_all_installed(available_pkgs);
	  for (i = 0; i < argc; i++) {
	       const char *target = argv[i];
	       int j;

	       opkg_msg(NOTICE, "What %s %s\n",
			    rel_str, target);
	       for (j = 0; j < available_pkgs->len; j++) {
		    pkg_t *pkg = available_pkgs->pkgs[j];
		    int k;
		    int count = (what_field_type == WHATPROVIDES) ? pkg->provides_count : pkg->replaces_count;
		    for (k = 0; k < count; k++) {
			 abstract_pkg_t *apkg =
			      ((what_field_type == WHATPROVIDES)
			       ? pkg->provides[k]
			       : pkg->replaces[k]);
			 if (fnmatch(target, apkg->name, 0) == 0) {
			      opkg_msg(NOTICE, "    %s", pkg->name);
			      if (strcmp(target, apkg->name) != 0)
				   opkg_msg(NOTICE, "\t%s %s\n",
						   rel_str, apkg->name);
			      opkg_message(NOTICE, "\n");
			 }
		    }
	       }
	  }
	  pkg_vec_free(available_pkgs);
     }
     return 0;
}

static int
opkg_whatprovides_cmd(int argc, char **argv)
{
     return opkg_what_provides_replaces_cmd(WHATPROVIDES, argc, argv);
}

static int
opkg_whatreplaces_cmd(int argc, char **argv)
{
     return opkg_what_provides_replaces_cmd(WHATREPLACES, argc, argv);
}

static int
opkg_search_cmd(int argc, char **argv)
{
     int i;

     pkg_vec_t *installed;
     pkg_t *pkg;
     str_list_t *installed_files;
     str_list_elt_t *iter;
     char *installed_file;

     if (argc < 1) {
	  return -1;
     }

     installed = pkg_vec_alloc();
     pkg_hash_fetch_all_installed(installed);
     pkg_vec_sort(installed, pkg_compare_names);

     for (i=0; i < installed->len; i++) {
	  pkg = installed->pkgs[i];

	  installed_files = pkg_get_installed_files(pkg);

	  for (iter = str_list_first(installed_files); iter; iter = str_list_next(installed_files, iter)) {
	       installed_file = (char *)iter->data;
	       if (fnmatch(argv[0], installed_file, 0)==0)
	            print_pkg(pkg);
	  }

	  pkg_free_installed_files(pkg);
     }

     pkg_vec_free(installed);

     return 0;
}

static int
opkg_compare_versions_cmd(int argc, char **argv)
{
     if (argc == 3) {
	  /* this is a bit gross */
	  struct pkg p1, p2;
	  parse_version(&p1, argv[0]);
	  parse_version(&p2, argv[2]);
	  return pkg_version_satisfied(&p1, &p2, argv[1]);
     } else {
	  opkg_msg(ERROR,
		       "opkg compare_versions <v1> <op> <v2>\n"
		       "<op> is one of <= >= << >> =\n");
	  return -1;
     }
}

static int
opkg_print_architecture_cmd(int argc, char **argv)
{
     nv_pair_list_elt_t *l;

     list_for_each_entry(l, &conf->arch_list.head, node) {
	  nv_pair_t *nv = (nv_pair_t *)l->data;
	  printf("arch %s %s\n", nv->name, nv->value);
     }
     return 0;
}


/* XXX: CLEANUP: The usage strings should be incorporated into this
   array for easier maintenance */
static opkg_cmd_t cmds[] = {
     {"update", 0, (opkg_cmd_fun_t)opkg_update_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"upgrade", 1, (opkg_cmd_fun_t)opkg_upgrade_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"list", 0, (opkg_cmd_fun_t)opkg_list_cmd, PFM_SOURCE},
     {"list_installed", 0, (opkg_cmd_fun_t)opkg_list_installed_cmd, PFM_SOURCE},
     {"list-installed", 0, (opkg_cmd_fun_t)opkg_list_installed_cmd, PFM_SOURCE},
     {"list_upgradable", 0, (opkg_cmd_fun_t)opkg_list_upgradable_cmd, PFM_SOURCE},
     {"list-upgradable", 0, (opkg_cmd_fun_t)opkg_list_upgradable_cmd, PFM_SOURCE},
     {"list_changed_conffiles", 0, (opkg_cmd_fun_t)opkg_list_changed_conffiles_cmd, PFM_SOURCE},
     {"list-changed-conffiles", 0, (opkg_cmd_fun_t)opkg_list_changed_conffiles_cmd, PFM_SOURCE},
     {"info", 0, (opkg_cmd_fun_t)opkg_info_cmd, 0},
     {"flag", 1, (opkg_cmd_fun_t)opkg_flag_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"status", 0, (opkg_cmd_fun_t)opkg_status_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"install", 1, (opkg_cmd_fun_t)opkg_install_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"remove", 1, (opkg_cmd_fun_t)opkg_remove_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"configure", 0, (opkg_cmd_fun_t)opkg_configure_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"files", 1, (opkg_cmd_fun_t)opkg_files_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"search", 1, (opkg_cmd_fun_t)opkg_search_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"download", 1, (opkg_cmd_fun_t)opkg_download_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"compare_versions", 1, (opkg_cmd_fun_t)opkg_compare_versions_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"compare-versions", 1, (opkg_cmd_fun_t)opkg_compare_versions_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"print-architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"print_architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"print-installation-architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"print_installation_architecture", 0, (opkg_cmd_fun_t)opkg_print_architecture_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"depends", 1, (opkg_cmd_fun_t)opkg_depends_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatdepends", 1, (opkg_cmd_fun_t)opkg_whatdepends_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatdependsrec", 1, (opkg_cmd_fun_t)opkg_whatdepends_recursively_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatrecommends", 1, (opkg_cmd_fun_t)opkg_whatrecommends_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatsuggests", 1, (opkg_cmd_fun_t)opkg_whatsuggests_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatprovides", 1, (opkg_cmd_fun_t)opkg_whatprovides_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatreplaces", 1, (opkg_cmd_fun_t)opkg_whatreplaces_cmd, PFM_DESCRIPTION|PFM_SOURCE},
     {"whatconflicts", 1, (opkg_cmd_fun_t)opkg_whatconflicts_cmd, PFM_DESCRIPTION|PFM_SOURCE},
};

opkg_cmd_t *
opkg_cmd_find(const char *name)
{
	int i;
	opkg_cmd_t *cmd;
	int num_cmds = sizeof(cmds) / sizeof(opkg_cmd_t);

	for (i=0; i < num_cmds; i++) {
		cmd = &cmds[i];
		if (strcmp(name, cmd->name) == 0)
			return cmd;
	}

	return NULL;
}

int
opkg_cmd_exec(opkg_cmd_t *cmd, int argc, const char **argv)
{
	return (cmd->fun)(argc, argv);
}
