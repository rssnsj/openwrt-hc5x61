/* opkg_conf.c - the opkg package management system

   Copyright (C) 2009 Ubiq Technologies <graham.gower@gmail.com>

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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>

#include "opkg_conf.h"
#include "pkg_vec.h"
#include "pkg.h"
#include "xregex.h"
#include "sprintf_alloc.h"
#include "opkg_message.h"
#include "file_util.h"
#include "opkg_defines.h"
#include "libbb/libbb.h"

static int lock_fd;
static char *lock_file = NULL;

static opkg_conf_t _conf;
opkg_conf_t *conf = &_conf;

/*
 * Config file options
 */
opkg_option_t options[] = {
	  { "cache", OPKG_OPT_TYPE_STRING, &_conf.cache},
	  { "force_defaults", OPKG_OPT_TYPE_BOOL, &_conf.force_defaults },
          { "force_maintainer", OPKG_OPT_TYPE_BOOL, &_conf.force_maintainer },
	  { "force_depends", OPKG_OPT_TYPE_BOOL, &_conf.force_depends },
	  { "force_overwrite", OPKG_OPT_TYPE_BOOL, &_conf.force_overwrite },
	  { "force_downgrade", OPKG_OPT_TYPE_BOOL, &_conf.force_downgrade },
	  { "force_reinstall", OPKG_OPT_TYPE_BOOL, &_conf.force_reinstall },
	  { "force_space", OPKG_OPT_TYPE_BOOL, &_conf.force_space },
	  { "force_postinstall", OPKG_OPT_TYPE_BOOL, &_conf.force_postinstall },
          { "check_signature", OPKG_OPT_TYPE_BOOL, &_conf.check_signature },
	  { "ftp_proxy", OPKG_OPT_TYPE_STRING, &_conf.ftp_proxy },
	  { "http_proxy", OPKG_OPT_TYPE_STRING, &_conf.http_proxy },
	  { "no_proxy", OPKG_OPT_TYPE_STRING, &_conf.no_proxy },
	  { "test", OPKG_OPT_TYPE_BOOL, &_conf.noaction },
	  { "noaction", OPKG_OPT_TYPE_BOOL, &_conf.noaction },
	  { "download_only", OPKG_OPT_TYPE_BOOL, &_conf.download_only },
	  { "nodeps", OPKG_OPT_TYPE_BOOL, &_conf.nodeps },
	  { "offline_root", OPKG_OPT_TYPE_STRING, &_conf.offline_root },
	  { "overlay_root", OPKG_OPT_TYPE_STRING, &_conf.overlay_root },
	  { "proxy_passwd", OPKG_OPT_TYPE_STRING, &_conf.proxy_passwd },
	  { "proxy_user", OPKG_OPT_TYPE_STRING, &_conf.proxy_user },
	  { "query-all", OPKG_OPT_TYPE_BOOL, &_conf.query_all },
	  { "tmp_dir", OPKG_OPT_TYPE_STRING, &_conf.tmp_dir },
	  { "verbosity", OPKG_OPT_TYPE_INT, &_conf.verbosity },
#if defined(HAVE_OPENSSL)
	  { "signature_ca_file", OPKG_OPT_TYPE_STRING, &_conf.signature_ca_file },
	  { "signature_ca_path", OPKG_OPT_TYPE_STRING, &_conf.signature_ca_path },
#endif
#if defined(HAVE_PATHFINDER)
          { "check_x509_path", OPKG_OPT_TYPE_BOOL, &_conf.check_x509_path },
#endif
#if defined(HAVE_SSLCURL) && defined(HAVE_CURL)
          { "ssl_engine", OPKG_OPT_TYPE_STRING, &_conf.ssl_engine },
          { "ssl_cert", OPKG_OPT_TYPE_STRING, &_conf.ssl_cert },
          { "ssl_cert_type", OPKG_OPT_TYPE_STRING, &_conf.ssl_cert_type },
          { "ssl_key", OPKG_OPT_TYPE_STRING, &_conf.ssl_key },
          { "ssl_key_type", OPKG_OPT_TYPE_STRING, &_conf.ssl_key_type },
          { "ssl_key_passwd", OPKG_OPT_TYPE_STRING, &_conf.ssl_key_passwd },
          { "ssl_ca_file", OPKG_OPT_TYPE_STRING, &_conf.ssl_ca_file },
          { "ssl_ca_path", OPKG_OPT_TYPE_STRING, &_conf.ssl_ca_path },
          { "ssl_dont_verify_peer", OPKG_OPT_TYPE_BOOL, &_conf.ssl_dont_verify_peer },
#endif
	  { NULL, 0, NULL }
};

static int
resolve_pkg_dest_list(void)
{
     nv_pair_list_elt_t *iter;
     nv_pair_t *nv_pair;
     pkg_dest_t *dest;
     char *root_dir;

     for (iter = nv_pair_list_first(&conf->tmp_dest_list); iter;
		     iter = nv_pair_list_next(&conf->tmp_dest_list, iter)) {
	  nv_pair = (nv_pair_t *)iter->data;

	  if (conf->offline_root) {
	       sprintf_alloc(&root_dir, "%s%s", conf->offline_root, nv_pair->value);
	  } else {
	       root_dir = xstrdup(nv_pair->value);
	  }

	  dest = pkg_dest_list_append(&conf->pkg_dest_list, nv_pair->name, root_dir, conf->lists_dir);
	  free(root_dir);

	  if (conf->default_dest == NULL)
	       conf->default_dest = dest;

	  if (conf->dest_str && !strcmp(dest->name, conf->dest_str)) {
	       conf->default_dest = dest;
	       conf->restrict_to_default_dest = 1;
	  }
     }

     if (conf->dest_str && !conf->restrict_to_default_dest) {
	  opkg_msg(ERROR, "Unknown dest name: `%s'.\n", conf->dest_str);
	  return -1;
     }

     return 0;
}

static int
opkg_conf_set_option(const char *name, const char *value)
{
     int i = 0;

     while (options[i].name) {
	  if (strcmp(options[i].name, name) == 0) {
	       switch (options[i].type) {
	       case OPKG_OPT_TYPE_BOOL:
		    if (*(int *)options[i].value) {
			    opkg_msg(ERROR, "Duplicate boolean option %s, "
				"leaving this option on.\n", name);
			    return 0;
		    }
		    *((int * const)options[i].value) = 1;
		    return 0;
	       case OPKG_OPT_TYPE_INT:
		    if (value) {
			    if (*(int *)options[i].value) {
				    opkg_msg(ERROR, "Duplicate option %s, "
					"using first seen value \"%d\".\n",
					name, *((int *)options[i].value));
				    return 0;
			    }
			 *((int * const)options[i].value) = atoi(value);
			 return 0;
		    } else {
			 opkg_msg(ERROR, "Option %s needs an argument\n",
				name);
			 return -1;
		    }
	       case OPKG_OPT_TYPE_STRING:
		    if (value) {
			    if (*(char **)options[i].value) {
				    opkg_msg(ERROR, "Duplicate option %s, "
					"using first seen value \"%s\".\n",
					name, *((char **)options[i].value));
				    return 0;
			    }
			 *((char ** const)options[i].value) = xstrdup(value);
			 return 0;
		    } else {
			 opkg_msg(ERROR, "Option %s needs an argument\n",
				name);
			 return -1;
		    }
	       }
	  }
	  i++;
     }

     opkg_msg(ERROR, "Unrecognized option: %s=%s\n", name, value);
     return -1;
}

static int
opkg_conf_parse_file(const char *filename,
				pkg_src_list_t *pkg_src_list,
				pkg_src_list_t *dist_src_list)
{
     int line_num = 0;
     int err = 0;
     FILE *file;
     regex_t valid_line_re, comment_re;
#define regmatch_size 14
     regmatch_t regmatch[regmatch_size];

     file = fopen(filename, "r");
     if (file == NULL) {
	  opkg_perror(ERROR, "Failed to open %s", filename);
	  err = -1;
	  goto err0;
     }

     opkg_msg(INFO, "Loading conf file %s.\n", filename);

     err = xregcomp(&comment_re,
		    "^[[:space:]]*(#.*|[[:space:]]*)$",
		    REG_EXTENDED);
     if (err)
          goto err1;

     err = xregcomp(&valid_line_re,
		     "^[[:space:]]*(\"([^\"]*)\"|([^[:space:]]*))"
		     "[[:space:]]*(\"([^\"]*)\"|([^[:space:]]*))"
		     "[[:space:]]*(\"([^\"]*)\"|([^[:space:]]*))"
		     "([[:space:]]+([^[:space:]]+))?([[:space:]]+(.*))?[[:space:]]*$",
		     REG_EXTENDED);
     if (err)
	  goto err2;

     while(1) {
	  char *line;
	  char *type, *name, *value, *extra;

	  line_num++;

	  line = file_read_line_alloc(file);
	  if (line == NULL)
	       break;

	  if (regexec(&comment_re, line, 0, 0, 0) == 0)
	       goto NEXT_LINE;

	  if (regexec(&valid_line_re, line, regmatch_size, regmatch, 0) == REG_NOMATCH) {
	       opkg_msg(ERROR, "%s:%d: Ignoring invalid line: `%s'\n",
		       filename, line_num, line);
	       goto NEXT_LINE;
	  }

	  /* This has to be so ugly to deal with optional quotation marks */
	  if (regmatch[2].rm_so > 0) {
	       type = xstrndup(line + regmatch[2].rm_so,
			      regmatch[2].rm_eo - regmatch[2].rm_so);
	  } else {
	       type = xstrndup(line + regmatch[3].rm_so,
			      regmatch[3].rm_eo - regmatch[3].rm_so);
	  }

	  if (regmatch[5].rm_so > 0) {
	       name = xstrndup(line + regmatch[5].rm_so,
			      regmatch[5].rm_eo - regmatch[5].rm_so);
	  } else {
	       name = xstrndup(line + regmatch[6].rm_so,
			      regmatch[6].rm_eo - regmatch[6].rm_so);
	  }

	  if (regmatch[8].rm_so > 0) {
	       value = xstrndup(line + regmatch[8].rm_so,
			       regmatch[8].rm_eo - regmatch[8].rm_so);
	  } else {
	       value = xstrndup(line + regmatch[9].rm_so,
			       regmatch[9].rm_eo - regmatch[9].rm_so);
	  }

	  extra = NULL;
	  if (regmatch[11].rm_so > 0) {
	     if (regmatch[13].rm_so > 0 && regmatch[13].rm_so!=regmatch[13].rm_eo )
	       extra = xstrndup (line + regmatch[11].rm_so,
				regmatch[13].rm_eo - regmatch[11].rm_so);
	     else
	       extra = xstrndup (line + regmatch[11].rm_so,
				regmatch[11].rm_eo - regmatch[11].rm_so);
	  }

	  if (regmatch[13].rm_so!=regmatch[13].rm_eo && strncmp(type, "dist", 4)!=0) {
	       opkg_msg(ERROR, "%s:%d: Ignoring config line with trailing garbage: `%s'\n",
		       filename, line_num, line);
	  } else {

	  /* We use the conf->tmp_dest_list below instead of
	     conf->pkg_dest_list because we might encounter an
	     offline_root option later and that would invalidate the
	     directories we would have computed in
	     pkg_dest_list_init. (We do a similar thing with
	     tmp_src_nv_pair_list for sake of symmetry.) */
	  if (strcmp(type, "option") == 0) {
	       opkg_conf_set_option(name, value);
 	  } else if (strcmp(type, "dist") == 0) {
 	       if (!nv_pair_list_find((nv_pair_list_t*) dist_src_list, name)) {
 		    pkg_src_list_append (dist_src_list, name, value, extra, 0);
 	       } else {
 		    opkg_msg(ERROR, "Duplicate dist declaration (%s %s). "
 				    "Skipping.\n", name, value);
 	       }
 	  } else if (strcmp(type, "dist/gz") == 0) {
 	       if (!nv_pair_list_find((nv_pair_list_t*) dist_src_list, name)) {
 		    pkg_src_list_append (dist_src_list, name, value, extra, 1);
 	       } else {
 		    opkg_msg(ERROR, "Duplicate dist declaration (%s %s). "
 				    "Skipping.\n", name, value);
 	       }
	  } else if (strcmp(type, "src") == 0) {
	       if (!nv_pair_list_find((nv_pair_list_t*) pkg_src_list, name)) {
		    pkg_src_list_append (pkg_src_list, name, value, extra, 0);
	       } else {
		    opkg_msg(ERROR, "Duplicate src declaration (%s %s). "
				    "Skipping.\n", name, value);
	       }
	  } else if (strcmp(type, "src/gz") == 0) {
	       if (!nv_pair_list_find((nv_pair_list_t*) pkg_src_list, name)) {
		    pkg_src_list_append (pkg_src_list, name, value, extra, 1);
	       } else {
		    opkg_msg(ERROR, "Duplicate src declaration (%s %s). "
				   "Skipping.\n", name, value);
	       }
	  } else if (strcmp(type, "dest") == 0) {
	       nv_pair_list_append(&conf->tmp_dest_list, name, value);
	  } else if (strcmp(type, "lists_dir") == 0) {
	       conf->lists_dir = xstrdup(value);
	  } else if (strcmp(type, "arch") == 0) {
	       opkg_msg(INFO, "Supported arch %s priority (%s)\n", name, value);
	       if (!value) {
		    opkg_msg(NOTICE, "No priority given for architecture %s,"
				   "defaulting to 10\n", name);
		    value = xstrdup("10");
	       }
	       nv_pair_list_append(&conf->arch_list, name, value);
	  } else {
	       opkg_msg(ERROR, "%s:%d: Ignoring invalid line: `%s'\n",
		       filename, line_num, line);
	  }

	  }

	  free(type);
	  free(name);
	  free(value);
	  if (extra)
	       free(extra);

NEXT_LINE:
	  free(line);
     }

     regfree(&valid_line_re);
err2:
     regfree(&comment_re);
err1:
     if (fclose(file) == EOF) {
          opkg_perror(ERROR, "Couldn't close %s", filename);
	  err = -1;
     }
err0:
     return err;
}

int
opkg_conf_write_status_files(void)
{
     pkg_dest_list_elt_t *iter;
     pkg_dest_t *dest;
     pkg_vec_t *all;
     pkg_t *pkg;
     int i, ret = 0;

     if (conf->noaction)
	  return 0;

     list_for_each_entry(iter, &conf->pkg_dest_list.head, node) {
          dest = (pkg_dest_t *)iter->data;

          dest->status_fp = fopen(dest->status_file_name, "w");
          if (dest->status_fp == NULL && errno != EROFS) {
               opkg_perror(ERROR, "Can't open status file %s",
                    dest->status_file_name);
               ret = -1;
          }
     }

     all = pkg_vec_alloc();
     pkg_hash_fetch_available(all);

     for(i = 0; i < all->len; i++) {
	  pkg = all->pkgs[i];
	  /* We don't need most uninstalled packages in the status file */
	  if (pkg->state_status == SS_NOT_INSTALLED
	      && (pkg->state_want == SW_UNKNOWN
		  || (pkg->state_want == SW_DEINSTALL
			  && pkg->state_flag != SF_HOLD)
		  || pkg->state_want == SW_PURGE)) {
	       continue;
	  }
	  if (pkg->dest == NULL) {
	       opkg_msg(ERROR, "Internal error: package %s has a NULL dest\n",
		       pkg->name);
	       continue;
	  }
	  if (pkg->dest->status_fp)
	       pkg_print_status(pkg, pkg->dest->status_fp);
     }

     pkg_vec_free(all);

     list_for_each_entry(iter, &conf->pkg_dest_list.head, node) {
          dest = (pkg_dest_t *)iter->data;
          if (dest->status_fp && fclose(dest->status_fp) == EOF) {
               opkg_perror(ERROR, "Couldn't close %s", dest->status_file_name);
	       ret = -1;
          }
     }

     return ret;
}


char *
root_filename_alloc(char *filename)
{
	char *root_filename;
	sprintf_alloc(&root_filename, "%s%s",
		(conf->offline_root ? conf->offline_root : ""), filename);
	return root_filename;
}

static int
glob_errfunc(const char *epath, int eerrno)
{
	if (eerrno == ENOENT)
		/* If leading dir does not exist, we get GLOB_NOMATCH. */
		return 0;

	opkg_msg(ERROR, "glob failed for %s: %s\n", epath, strerror(eerrno));
	return 0;
}

int
opkg_conf_init(void)
{
	pkg_src_list_init(&conf->pkg_src_list);
	pkg_src_list_init(&conf->dist_src_list);
	pkg_dest_list_init(&conf->pkg_dest_list);
	pkg_dest_list_init(&conf->tmp_dest_list);
	nv_pair_list_init(&conf->arch_list);

	return 0;
}

int
opkg_conf_load(void)
{
	int i, glob_ret;
	char *tmp, *tmp_dir_base, **tmp_val;
	glob_t globbuf;
	char *etc_opkg_conf_pattern;

	conf->restrict_to_default_dest = 0;
	conf->default_dest = NULL;
#if defined(HAVE_PATHFINDER)
	conf->check_x509_path = 1;
#endif

	if (!conf->offline_root)
		conf->offline_root = xstrdup(getenv("OFFLINE_ROOT"));

	if (conf->conf_file) {
		struct stat st;
		if (stat(conf->conf_file, &st) == -1) {
			opkg_perror(ERROR, "Couldn't stat %s", conf->conf_file);
			goto err0;
		}
		if (opkg_conf_parse_file(conf->conf_file,
				&conf->pkg_src_list, &conf->dist_src_list))
			goto err1;
	}

	if (conf->offline_root)
		sprintf_alloc(&etc_opkg_conf_pattern, "%s/etc/opkg/*.conf", conf->offline_root);
	else {
		const char *conf_file_dir = getenv("OPKG_CONF_DIR");
		if (conf_file_dir == NULL)
			conf_file_dir = OPKG_CONF_DEFAULT_CONF_FILE_DIR;
			sprintf_alloc(&etc_opkg_conf_pattern, "%s/*.conf", conf_file_dir);
	}

	memset(&globbuf, 0, sizeof(globbuf));
	glob_ret = glob(etc_opkg_conf_pattern, 0, glob_errfunc, &globbuf);
	if (glob_ret && glob_ret != GLOB_NOMATCH) {
		free(etc_opkg_conf_pattern);
		globfree(&globbuf);
		goto err1;
	}

	free(etc_opkg_conf_pattern);

	for (i = 0; i < globbuf.gl_pathc; i++) {
		if (globbuf.gl_pathv[i])
			if (conf->conf_file &&
					!strcmp(conf->conf_file, globbuf.gl_pathv[i]))
				continue;
		if ( opkg_conf_parse_file(globbuf.gl_pathv[i],
			&conf->pkg_src_list, &conf->dist_src_list)<0) {
			globfree(&globbuf);
			goto err1;
		}
	}

	globfree(&globbuf);

	if (conf->offline_root)
		sprintf_alloc (&lock_file, "%s/%s", conf->offline_root, OPKGLOCKFILE);
	else
		sprintf_alloc (&lock_file, "%s", OPKGLOCKFILE);

	lock_fd = creat(lock_file, S_IRUSR | S_IWUSR | S_IRGRP);
	if (lock_fd == -1) {
		opkg_perror(ERROR, "Could not create lock file %s", lock_file);
		goto err2;
	}

	if (lockf(lock_fd, F_TLOCK, (off_t)0) == -1) {
		opkg_perror(ERROR, "Could not lock %s", lock_file);
		if (close(lock_fd) == -1)
			opkg_perror(ERROR, "Couldn't close descriptor %d (%s)",
				lock_fd, lock_file);
		lock_fd = -1;
		goto err2;
	}

	if (conf->tmp_dir)
		tmp_dir_base = conf->tmp_dir;
	else
		tmp_dir_base = getenv("TMPDIR");

	sprintf_alloc(&tmp, "%s/%s",
		tmp_dir_base ? tmp_dir_base : OPKG_CONF_DEFAULT_TMP_DIR_BASE,
		OPKG_CONF_TMP_DIR_SUFFIX);
	if (conf->tmp_dir)
		free(conf->tmp_dir);
	conf->tmp_dir = mkdtemp(tmp);
	if (conf->tmp_dir == NULL) {
		opkg_perror(ERROR, "Creating temp dir %s failed", tmp);
		goto err3;
	}

	pkg_hash_init();
	hash_table_init("file-hash", &conf->file_hash, OPKG_CONF_DEFAULT_HASH_LEN);
	hash_table_init("obs-file-hash", &conf->obs_file_hash, OPKG_CONF_DEFAULT_HASH_LEN/16);

	if (conf->lists_dir == NULL)
		conf->lists_dir = xstrdup(OPKG_CONF_LISTS_DIR);

	if (conf->offline_root) {
		sprintf_alloc(&tmp, "%s/%s", conf->offline_root, conf->lists_dir);
		free(conf->lists_dir);
		conf->lists_dir = tmp;
	}

	/* if no architectures were defined, then default all, noarch, and host architecture */
	if (nv_pair_list_empty(&conf->arch_list)) {
		nv_pair_list_append(&conf->arch_list, "all", "1");
		nv_pair_list_append(&conf->arch_list, "noarch", "1");
		nv_pair_list_append(&conf->arch_list, HOST_CPU_STR, "10");
	}

	/* Even if there is no conf file, we'll need at least one dest. */
	if (nv_pair_list_empty(&conf->tmp_dest_list)) {
		nv_pair_list_append(&conf->tmp_dest_list,
			OPKG_CONF_DEFAULT_DEST_NAME,
			OPKG_CONF_DEFAULT_DEST_ROOT_DIR);
	}

	if (resolve_pkg_dest_list())
		goto err4;

	nv_pair_list_deinit(&conf->tmp_dest_list);

	return 0;


err4:
	free(conf->lists_dir);

	pkg_hash_deinit();
	hash_table_deinit(&conf->file_hash);
	hash_table_deinit(&conf->obs_file_hash);

	if (rmdir(conf->tmp_dir) == -1)
		opkg_perror(ERROR, "Couldn't remove dir %s", conf->tmp_dir);
err3:
	if (lockf(lock_fd, F_ULOCK, (off_t)0) == -1)
		opkg_perror(ERROR, "Couldn't unlock %s", lock_file);

	if (close(lock_fd) == -1)
		opkg_perror(ERROR, "Couldn't close descriptor %d (%s)",
				lock_fd, lock_file);
	if (unlink(lock_file) == -1)
		opkg_perror(ERROR, "Couldn't unlink %s", lock_file);
err2:
	if (lock_file) {
		free(lock_file);
		lock_file = NULL;
	}
err1:
	pkg_src_list_deinit(&conf->pkg_src_list);
	pkg_src_list_deinit(&conf->dist_src_list);
	pkg_dest_list_deinit(&conf->pkg_dest_list);
	nv_pair_list_deinit(&conf->arch_list);

	for (i=0; options[i].name; i++) {
		if (options[i].type == OPKG_OPT_TYPE_STRING) {
			tmp_val = (char **)options[i].value;
			if (*tmp_val) {
				free(*tmp_val);
				*tmp_val = NULL;
			}
		}
	}
err0:
	nv_pair_list_deinit(&conf->tmp_dest_list);
	if (conf->dest_str)
		free(conf->dest_str);
	if (conf->conf_file)
		free(conf->conf_file);

	return -1;
}

void
opkg_conf_deinit(void)
{
	int i;
	char **tmp;

	if (conf->tmp_dir)
		rm_r(conf->tmp_dir);

	if (conf->lists_dir)
		free(conf->lists_dir);

	if (conf->dest_str)
		free(conf->dest_str);

	if (conf->conf_file)
		free(conf->conf_file);

	pkg_src_list_deinit(&conf->pkg_src_list);
	pkg_src_list_deinit(&conf->dist_src_list);
	pkg_dest_list_deinit(&conf->pkg_dest_list);
	nv_pair_list_deinit(&conf->arch_list);

	for (i=0; options[i].name; i++) {
		if (options[i].type == OPKG_OPT_TYPE_STRING) {
			tmp = (char **)options[i].value;
			if (*tmp) {
				free(*tmp);
				*tmp = NULL;
			}
		}
	}

	if (conf->verbosity >= DEBUG) {
		hash_print_stats(&conf->pkg_hash);
		hash_print_stats(&conf->file_hash);
		hash_print_stats(&conf->obs_file_hash);
	}

	pkg_hash_deinit();
	hash_table_deinit(&conf->file_hash);
	hash_table_deinit(&conf->obs_file_hash);

	if (lock_fd != -1) {
		if (lockf(lock_fd, F_ULOCK, (off_t)0) == -1)
			opkg_perror(ERROR, "Couldn't unlock %s", lock_file);

		if (close(lock_fd) == -1)
			opkg_perror(ERROR, "Couldn't close descriptor %d (%s)",
					lock_fd, lock_file);

	}

	if (lock_file) {
		if (unlink(lock_file) == -1)
			opkg_perror(ERROR, "Couldn't unlink %s", lock_file);

		free(lock_file);
	}
}
