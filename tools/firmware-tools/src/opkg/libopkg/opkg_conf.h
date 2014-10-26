/* opkg_conf.h - the opkg package management system

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

#ifndef OPKG_CONF_H
#define OPKG_CONF_H

typedef struct opkg_conf opkg_conf_t;
extern opkg_conf_t *conf;

#include "config.h"

#include <stdarg.h>

#include "hash_table.h"
#include "pkg_src_list.h"
#include "pkg_dest_list.h"
#include "nv_pair_list.h"

#define OPKG_CONF_DEFAULT_TMP_DIR_BASE "/tmp"
#define OPKG_CONF_TMP_DIR_SUFFIX "opkg-XXXXXX"
#define OPKG_CONF_LISTS_DIR  OPKG_STATE_DIR_PREFIX "/lists"

#define OPKG_CONF_DEFAULT_CONF_FILE_DIR OPKGETCDIR"/opkg"

/* In case the config file defines no dest */
#define OPKG_CONF_DEFAULT_DEST_NAME "root"
#define OPKG_CONF_DEFAULT_DEST_ROOT_DIR "/"

#define OPKG_CONF_DEFAULT_HASH_LEN 1024

struct opkg_conf
{
     pkg_src_list_t pkg_src_list;
     pkg_src_list_t dist_src_list;
     pkg_dest_list_t pkg_dest_list;
     pkg_dest_list_t tmp_dest_list;
     nv_pair_list_t arch_list;

     int restrict_to_default_dest;
     pkg_dest_t *default_dest;
     char *dest_str;

     char *conf_file;

     char *tmp_dir;
     char *lists_dir;

     unsigned int pfm; /* package field mask */

     /* For libopkg users to capture messages. */
     void (*opkg_vmessage)(int, const char *fmt, va_list ap);

     /* options */
     int autoremove;
     int force_depends;
     int force_defaults;
     int force_maintainer;
     int force_overwrite;
     int force_downgrade;
     int force_reinstall;
     int force_space;
     int force_removal_of_dependent_packages;
     int force_removal_of_essential_packages;
     int force_postinstall;
     int force_remove;
     int check_signature;
     int nodeps; /* do not follow dependencies */
     char *offline_root;
     char *overlay_root;
     int query_all;
     int verbosity;
     int noaction;
     int download_only;
     char *cache;

#ifdef HAVE_SSLCURL
     /* some options could be used by
      * wget if curl support isn't builtin
      * If someone want to try...
      */
     char *ssl_engine;
     char *ssl_cert;
     char *ssl_cert_type;
     char *ssl_key;
     char *ssl_key_type;
     char *ssl_key_passwd;
     char *ssl_ca_file;
     char *ssl_ca_path;
     int ssl_dont_verify_peer;
#endif
#ifdef HAVE_PATHFINDER
     int check_x509_path;
#endif

     /* proxy options */
     char *http_proxy;
     char *ftp_proxy;
     char *no_proxy;
     char *proxy_user;
     char *proxy_passwd;

     char *signature_ca_file;
     char *signature_ca_path;

     hash_table_t pkg_hash;
     hash_table_t file_hash;
     hash_table_t obs_file_hash;
};

enum opkg_option_type {
     OPKG_OPT_TYPE_BOOL,
     OPKG_OPT_TYPE_INT,
     OPKG_OPT_TYPE_STRING
};
typedef enum opkg_option_type opkg_option_type_t;

typedef struct opkg_option opkg_option_t;
struct opkg_option {
     const char *name;
     const opkg_option_type_t type;
     void * const value;
};

int opkg_conf_init(void);
int opkg_conf_load(void);
void opkg_conf_deinit(void);

int opkg_conf_write_status_files(void);
char *root_filename_alloc(char *filename);

#endif
