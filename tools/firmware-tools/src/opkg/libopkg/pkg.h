/* pkg.h - the opkg package management system

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

#ifndef PKG_H
#define PKG_H

#include "config.h"

#include <sys/types.h>

#include "pkg_vec.h"
#include "str_list.h"
#include "active_list.h"
#include "pkg_src.h"
#include "pkg_dest.h"
#include "opkg_conf.h"
#include "conffile_list.h"

struct opkg_conf;


#define ARRAY_SIZE(array) sizeof(array) / sizeof((array)[0])

/* I think "Size" is currently the shortest field name */
#define PKG_MINIMUM_FIELD_NAME_LEN 4

enum pkg_state_want
{
    SW_UNKNOWN = 1,
    SW_INSTALL,
    SW_DEINSTALL,
    SW_PURGE,
    SW_LAST_STATE_WANT
};
typedef enum pkg_state_want pkg_state_want_t;

enum pkg_state_flag
{
    SF_OK        = 0,
    SF_REINSTREQ = 1,
    SF_HOLD      = 2,  /* do not upgrade version */
    SF_REPLACE   = 4,  /* replace this package */
    SF_NOPRUNE   = 8,  /* do not remove obsolete files */
    SF_PREFER    = 16, /* prefer this version */
    SF_OBSOLETE  = 32, /* old package in upgrade pair */
    SF_MARKED    = 64, /* temporary mark */
    SF_FILELIST_CHANGED = 128, /* needs filelist written */
    SF_USER      = 256,
    SF_LAST_STATE_FLAG
};
typedef enum pkg_state_flag pkg_state_flag_t;
#define SF_NONVOLATILE_FLAGS (SF_HOLD|SF_NOPRUNE|SF_PREFER|SF_OBSOLETE|SF_USER)

enum pkg_state_status
{
    SS_NOT_INSTALLED = 1,
    SS_UNPACKED,
    SS_HALF_CONFIGURED,
    SS_INSTALLED,
    SS_HALF_INSTALLED,
    SS_CONFIG_FILES,
    SS_POST_INST_FAILED,
    SS_REMOVAL_FAILED,
    SS_LAST_STATE_STATUS
};
typedef enum pkg_state_status pkg_state_status_t;

struct abstract_pkg{
    char * name;
    int dependencies_checked;
    pkg_vec_t * pkgs;
    pkg_state_status_t state_status;
    pkg_state_flag_t state_flag;

    /* XXX: This should be abstract_pkg_vec_t for consistency. */
    struct abstract_pkg ** depended_upon_by;

    abstract_pkg_vec_t * provided_by;
    abstract_pkg_vec_t * replaced_by;
};

#include "pkg_depends.h"

/* XXX: CLEANUP: I'd like to clean up pkg_t in several ways:

   The 3 version fields should go into a single version struct. (This
   is especially important since, currently, pkg->version can easily
   be mistaken for pkg_verson_str_alloc(pkg) although they are very
   distinct. This has been the source of multiple bugs.

   The 3 state fields could possibly also go into their own struct.

   All fields which deal with lists of packages, (Depends,
   Pre-Depends, Provides, Suggests, Recommends, Enhances), should each
   be handled by a single struct in pkg_t

   All string fields for which there is a small set of possible
   values, (section, maintainer, architecture, maybe version?), that
   are reused among different packages -- for all such packages we
   should move from "char *"s to some atom datatype to share data
   storage and use less memory. We might even do reference counting,
   but probably not since most often we only create new pkg_t structs,
   we don't often free them.  */
struct pkg
{
     char *name;
     unsigned long epoch;
     char *version;
     char *revision;
     pkg_src_t *src;
     pkg_dest_t *dest;
     char *architecture;
     char *section;
     char *maintainer;
     char *description;
     char *tags;
     pkg_state_want_t state_want;
     pkg_state_flag_t state_flag;
     pkg_state_status_t state_status;
     char **depends_str;
     unsigned int depends_count;
     char **pre_depends_str;
     unsigned int pre_depends_count;
     char **recommends_str;
     unsigned int recommends_count;
     char **suggests_str;
     unsigned int suggests_count;
     struct active_list list; /* Used for installing|upgrading */
     compound_depend_t * depends;

     char **conflicts_str;
     compound_depend_t * conflicts;
     unsigned int conflicts_count;

     char **replaces_str;
     unsigned int replaces_count;
     abstract_pkg_t ** replaces;

     char **provides_str;
     unsigned int provides_count;
     abstract_pkg_t ** provides;

     abstract_pkg_t *parent;

     char *filename;
     char *local_filename;
     char *tmp_unpack_dir;
     char *md5sum;
#if defined HAVE_SHA256
     char *sha256sum;
#endif
     unsigned long size;		/* in bytes */
     unsigned long installed_size;	/* in bytes */
     char *priority;
     char *source;
     conffile_list_t conffiles;
     time_t installed_time;
     /* As pointer for lazy evaluation */
     str_list_t *installed_files;
     /* XXX: CLEANUP: I'd like to perhaps come up with a better
	mechanism to avoid the problem here, (which is that the
	installed_files list was being freed from an inner loop while
	still being used within an outer loop. */
     int installed_files_ref_cnt;
     int essential;
     int arch_priority;
/* Adding this flag, to "force" opkg to choose a "provided_by_hand" package, if there are multiple choice */
     int provided_by_hand;

     /* this flag specifies whether the package was installed to satisfy another
      * package's dependancies */
     int auto_installed;
};

pkg_t *pkg_new(void);
void pkg_deinit(pkg_t *pkg);
int pkg_init_from_file(pkg_t *pkg, const char *filename);
abstract_pkg_t *abstract_pkg_new(void);

/*
 * merges fields from newpkg into oldpkg.
 * Forcibly sets oldpkg state_status, state_want and state_flags
 */
int pkg_merge(pkg_t *oldpkg, pkg_t *newpkg);

char *pkg_version_str_alloc(pkg_t *pkg);

int pkg_compare_versions(const pkg_t *pkg, const pkg_t *ref_pkg);
int pkg_name_version_and_architecture_compare(const void *a, const void *b);
int abstract_pkg_name_compare(const void *a, const void *b);

void pkg_formatted_info(FILE *fp, pkg_t *pkg);
void pkg_formatted_field(FILE *fp, pkg_t *pkg, const char *field);

void set_flags_from_control(pkg_t *pkg);

void pkg_print_status(pkg_t * pkg, FILE * file);
str_list_t *pkg_get_installed_files(pkg_t *pkg);
void pkg_free_installed_files(pkg_t *pkg);
void pkg_remove_installed_files_list(pkg_t *pkg);
conffile_t *pkg_get_conffile(pkg_t *pkg, const char *file_name);
int pkg_run_script(pkg_t *pkg, const char *script, const char *args);

/* enum mappings */
pkg_state_want_t pkg_state_want_from_str(char *str);
pkg_state_flag_t pkg_state_flag_from_str(const char *str);
pkg_state_status_t pkg_state_status_from_str(const char *str);

int pkg_version_satisfied(pkg_t *it, pkg_t *ref, const char *op);

int pkg_arch_supported(pkg_t *pkg);
void pkg_info_preinstall_check(void);

int pkg_write_filelist(pkg_t *pkg);
int pkg_write_changed_filelists(void);

#endif
