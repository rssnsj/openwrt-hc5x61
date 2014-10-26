/* pkg.c - the opkg package management system

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
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>

#include "pkg.h"

#include "pkg_parse.h"
#include "pkg_extract.h"
#include "opkg_message.h"
#include "opkg_utils.h"

#include "libbb/libbb.h"
#include "sprintf_alloc.h"
#include "file_util.h"
#include "xsystem.h"
#include "opkg_conf.h"

typedef struct enum_map enum_map_t;
struct enum_map
{
     unsigned int value;
     const char *str;
};

static const enum_map_t pkg_state_want_map[] = {
     { SW_UNKNOWN, "unknown"},
     { SW_INSTALL, "install"},
     { SW_DEINSTALL, "deinstall"},
     { SW_PURGE, "purge"}
};

static const enum_map_t pkg_state_flag_map[] = {
     { SF_OK, "ok"},
     { SF_REINSTREQ, "reinstreq"},
     { SF_HOLD, "hold"},
     { SF_REPLACE, "replace"},
     { SF_NOPRUNE, "noprune"},
     { SF_PREFER, "prefer"},
     { SF_OBSOLETE, "obsolete"},
     { SF_USER, "user"},
};

static const enum_map_t pkg_state_status_map[] = {
     { SS_NOT_INSTALLED, "not-installed" },
     { SS_UNPACKED, "unpacked" },
     { SS_HALF_CONFIGURED, "half-configured" },
     { SS_INSTALLED, "installed" },
     { SS_HALF_INSTALLED, "half-installed" },
     { SS_CONFIG_FILES, "config-files" },
     { SS_POST_INST_FAILED, "post-inst-failed" },
     { SS_REMOVAL_FAILED, "removal-failed" }
};

static void
pkg_init(pkg_t *pkg)
{
     pkg->name = NULL;
     pkg->epoch = 0;
     pkg->version = NULL;
     pkg->revision = NULL;
     pkg->dest = NULL;
     pkg->src = NULL;
     pkg->architecture = NULL;
     pkg->maintainer = NULL;
     pkg->section = NULL;
     pkg->description = NULL;
     pkg->state_want = SW_UNKNOWN;
     pkg->state_flag = SF_OK;
     pkg->state_status = SS_NOT_INSTALLED;
     pkg->depends_str = NULL;
     pkg->provides_str = NULL;
     pkg->depends_count = 0;
     pkg->depends = NULL;
     pkg->suggests_str = NULL;
     pkg->recommends_str = NULL;
     pkg->suggests_count = 0;
     pkg->recommends_count = 0;

     active_list_init(&pkg->list);

     pkg->conflicts = NULL;
     pkg->conflicts_count = 0;

     pkg->replaces = NULL;
     pkg->replaces_count = 0;

     pkg->pre_depends_count = 0;
     pkg->pre_depends_str = NULL;
     pkg->provides_count = 0;
     pkg->provides = NULL;
     pkg->filename = NULL;
     pkg->local_filename = NULL;
     pkg->tmp_unpack_dir = NULL;
     pkg->md5sum = NULL;
#if defined HAVE_SHA256
     pkg->sha256sum = NULL;
#endif
     pkg->size = 0;
     pkg->installed_size = 0;
     pkg->priority = NULL;
     pkg->source = NULL;
     conffile_list_init(&pkg->conffiles);
     pkg->installed_files = NULL;
     pkg->installed_files_ref_cnt = 0;
     pkg->essential = 0;
     pkg->provided_by_hand = 0;
}

pkg_t *
pkg_new(void)
{
     pkg_t *pkg;

     pkg = xcalloc(1, sizeof(pkg_t));
     pkg_init(pkg);

     return pkg;
}

static void
compound_depend_deinit(compound_depend_t *depends)
{
    int i;
    for (i = 0; i < depends->possibility_count; i++)
    {
        depend_t *d;
        d = depends->possibilities[i];
        free (d->version);
        free (d);
    }
    free (depends->possibilities);
}

void
pkg_deinit(pkg_t *pkg)
{
	int i;

	if (pkg->name)
		free(pkg->name);
	pkg->name = NULL;

	pkg->epoch = 0;

	if (pkg->version)
		free(pkg->version);
	pkg->version = NULL;
	/* revision shares storage with version, so don't free */
	pkg->revision = NULL;

	/* owned by opkg_conf_t */
	pkg->dest = NULL;
	/* owned by opkg_conf_t */
	pkg->src = NULL;

	if (pkg->architecture)
		free(pkg->architecture);
	pkg->architecture = NULL;

	if (pkg->maintainer)
		free(pkg->maintainer);
	pkg->maintainer = NULL;

	if (pkg->section)
		free(pkg->section);
	pkg->section = NULL;

	if (pkg->description)
		free(pkg->description);
	pkg->description = NULL;

	pkg->state_want = SW_UNKNOWN;
	pkg->state_flag = SF_OK;
	pkg->state_status = SS_NOT_INSTALLED;

	active_list_clear(&pkg->list);

	if (pkg->replaces)
		free (pkg->replaces);
	pkg->replaces = NULL;

	if (pkg->depends) {
		int count = pkg->pre_depends_count
				+ pkg->depends_count
				+ pkg->recommends_count
				+ pkg->suggests_count;

		for (i=0; i<count; i++)
			compound_depend_deinit (&pkg->depends[i]);
		free (pkg->depends);
	}

	if (pkg->conflicts) {
		for (i=0; i<pkg->conflicts_count; i++)
			compound_depend_deinit (&pkg->conflicts[i]);
		free (pkg->conflicts);
	}

	if (pkg->provides)
		free (pkg->provides);

	pkg->pre_depends_count = 0;
	pkg->provides_count = 0;

	if (pkg->filename)
		free(pkg->filename);
	pkg->filename = NULL;

	if (pkg->local_filename)
		free(pkg->local_filename);
	pkg->local_filename = NULL;

     /* CLEANUP: It'd be nice to pullin the cleanup function from
	opkg_install.c here. See comment in
	opkg_install.c:cleanup_temporary_files */
	if (pkg->tmp_unpack_dir)
		free(pkg->tmp_unpack_dir);
	pkg->tmp_unpack_dir = NULL;

	if (pkg->md5sum)
		free(pkg->md5sum);
	pkg->md5sum = NULL;

#if defined HAVE_SHA256
	if (pkg->sha256sum)
		free(pkg->sha256sum);
	pkg->sha256sum = NULL;
#endif

	if (pkg->priority)
		free(pkg->priority);
	pkg->priority = NULL;

	if (pkg->source)
		free(pkg->source);
	pkg->source = NULL;

	conffile_list_deinit(&pkg->conffiles);

	/* XXX: QUESTION: Is forcing this to 1 correct? I suppose so,
	since if they are calling deinit, they should know. Maybe do an
	assertion here instead? */
	pkg->installed_files_ref_cnt = 1;
	pkg_free_installed_files(pkg);
	pkg->essential = 0;

	if (pkg->tags)
		free (pkg->tags);
	pkg->tags = NULL;
}

int
pkg_init_from_file(pkg_t *pkg, const char *filename)
{
	int fd, err = 0;
	FILE *control_file;
	char *control_path, *tmp;

	pkg_init(pkg);

	pkg->local_filename = xstrdup(filename);

	tmp = xstrdup(filename);
	sprintf_alloc(&control_path, "%s/%s.control.XXXXXX",
                        conf->tmp_dir,
                        basename(tmp));
	free(tmp);
	fd = mkstemp(control_path);
	if (fd == -1) {
		opkg_perror(ERROR, "Failed to make temp file %s", control_path);
		err = -1;
		goto err0;
	}

	control_file = fdopen(fd, "r+");
	if (control_file == NULL) {
		opkg_perror(ERROR, "Failed to fdopen %s", control_path);
		close(fd);
		err = -1;
		goto err1;
	}

	err = pkg_extract_control_file_to_stream(pkg, control_file);
	if (err) {
		opkg_msg(ERROR, "Failed to extract control file from %s.\n",
				filename);
		goto err2;
	}

	rewind(control_file);

	if ((err = pkg_parse_from_stream(pkg, control_file, 0))) {
		if (err == 1) {
			opkg_msg(ERROR, "Malformed package file %s.\n",
				filename);
		}
		err = -1;
	}

err2:
	fclose(control_file);
err1:
	unlink(control_path);
err0:
	free(control_path);

	return err;
}

/* Merge any new information in newpkg into oldpkg */
int
pkg_merge(pkg_t *oldpkg, pkg_t *newpkg)
{
     if (oldpkg == newpkg) {
	  return 0;
     }

     if (!oldpkg->auto_installed)
	  oldpkg->auto_installed = newpkg->auto_installed;

     if (!oldpkg->src)
	  oldpkg->src = newpkg->src;
     if (!oldpkg->dest)
	  oldpkg->dest = newpkg->dest;
     if (!oldpkg->architecture)
	  oldpkg->architecture = xstrdup(newpkg->architecture);
     if (!oldpkg->arch_priority)
	  oldpkg->arch_priority = newpkg->arch_priority;
     if (!oldpkg->section)
	  oldpkg->section = xstrdup(newpkg->section);
     if(!oldpkg->maintainer)
	  oldpkg->maintainer = xstrdup(newpkg->maintainer);
     if(!oldpkg->description)
	  oldpkg->description = xstrdup(newpkg->description);

     if (!oldpkg->depends_count && !oldpkg->pre_depends_count && !oldpkg->recommends_count && !oldpkg->suggests_count) {
	  oldpkg->depends_count = newpkg->depends_count;
	  newpkg->depends_count = 0;

	  oldpkg->depends = newpkg->depends;
	  newpkg->depends = NULL;

	  oldpkg->pre_depends_count = newpkg->pre_depends_count;
	  newpkg->pre_depends_count = 0;

	  oldpkg->recommends_count = newpkg->recommends_count;
	  newpkg->recommends_count = 0;

	  oldpkg->suggests_count = newpkg->suggests_count;
	  newpkg->suggests_count = 0;
     }

     if (oldpkg->provides_count <= 1) {
	  oldpkg->provides_count = newpkg->provides_count;
	  newpkg->provides_count = 0;

	  if (!oldpkg->provides) {
		oldpkg->provides = newpkg->provides;
		newpkg->provides = NULL;
	  }
     }

     if (!oldpkg->conflicts_count) {
	  oldpkg->conflicts_count = newpkg->conflicts_count;
	  newpkg->conflicts_count = 0;

	  oldpkg->conflicts = newpkg->conflicts;
	  newpkg->conflicts = NULL;
     }

     if (!oldpkg->replaces_count) {
	  oldpkg->replaces_count = newpkg->replaces_count;
	  newpkg->replaces_count = 0;

	  oldpkg->replaces = newpkg->replaces;
	  newpkg->replaces = NULL;
     }

     if (!oldpkg->filename)
	  oldpkg->filename = xstrdup(newpkg->filename);
     if (!oldpkg->local_filename)
	  oldpkg->local_filename = xstrdup(newpkg->local_filename);
     if (!oldpkg->tmp_unpack_dir)
	  oldpkg->tmp_unpack_dir = xstrdup(newpkg->tmp_unpack_dir);
     if (!oldpkg->md5sum)
	  oldpkg->md5sum = xstrdup(newpkg->md5sum);
#if defined HAVE_SHA256
     if (!oldpkg->sha256sum)
	  oldpkg->sha256sum = xstrdup(newpkg->sha256sum);
#endif
     if (!oldpkg->size)
	  oldpkg->size = newpkg->size;
     if (!oldpkg->installed_size)
	  oldpkg->installed_size = newpkg->installed_size;
     if (!oldpkg->priority)
	  oldpkg->priority = xstrdup(newpkg->priority);
     if (!oldpkg->source)
	  oldpkg->source = xstrdup(newpkg->source);

     if (nv_pair_list_empty(&oldpkg->conffiles)){
	  list_splice_init(&newpkg->conffiles.head, &oldpkg->conffiles.head);
     }

     if (!oldpkg->installed_files){
	  oldpkg->installed_files = newpkg->installed_files;
	  oldpkg->installed_files_ref_cnt = newpkg->installed_files_ref_cnt;
	  newpkg->installed_files = NULL;
     }

     if (!oldpkg->essential)
	  oldpkg->essential = newpkg->essential;

     return 0;
}

static void
abstract_pkg_init(abstract_pkg_t *ab_pkg)
{
     ab_pkg->provided_by = abstract_pkg_vec_alloc();
     ab_pkg->dependencies_checked = 0;
     ab_pkg->state_status = SS_NOT_INSTALLED;
}

abstract_pkg_t *
abstract_pkg_new(void)
{
     abstract_pkg_t * ab_pkg;

     ab_pkg = xcalloc(1, sizeof(abstract_pkg_t));
     abstract_pkg_init(ab_pkg);

     return ab_pkg;
}

void
set_flags_from_control(pkg_t *pkg){
     char *file_name;
     FILE *fp;

     sprintf_alloc(&file_name,"%s/%s.control", pkg->dest->info_dir, pkg->name);

     fp = fopen(file_name, "r");
     if (fp == NULL) {
	     opkg_perror(ERROR, "Failed to open %s", file_name);
	     free(file_name);
	     return;
     }

     free(file_name);

     if (pkg_parse_from_stream(pkg, fp, PFM_ALL ^ PFM_ESSENTIAL)) {
        opkg_msg(DEBUG, "Unable to read control file for %s. May be empty.\n",
			pkg->name);
     }

     fclose(fp);

     return;
}

static const char *
pkg_state_want_to_str(pkg_state_want_t sw)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_want_map); i++) {
	  if (pkg_state_want_map[i].value == sw) {
	       return pkg_state_want_map[i].str;
	  }
     }

     opkg_msg(ERROR, "Internal error: state_want=%d\n", sw);
     return "<STATE_WANT_UNKNOWN>";
}

pkg_state_want_t
pkg_state_want_from_str(char *str)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_want_map); i++) {
	  if (strcmp(str, pkg_state_want_map[i].str) == 0) {
	       return pkg_state_want_map[i].value;
	  }
     }

     opkg_msg(ERROR, "Internal error: state_want=%s\n", str);
     return SW_UNKNOWN;
}

static char *
pkg_state_flag_to_str(pkg_state_flag_t sf)
{
	int i;
	unsigned int len;
	char *str;

	/* clear the temporary flags before converting to string */
	sf &= SF_NONVOLATILE_FLAGS;

	if (sf == 0)
		return xstrdup("ok");

	len = 0;
	for (i=0; i < ARRAY_SIZE(pkg_state_flag_map); i++) {
		if (sf & pkg_state_flag_map[i].value)
			len += strlen(pkg_state_flag_map[i].str) + 1;
	}

	str = xmalloc(len+1);
	str[0] = '\0';

	for (i=0; i < ARRAY_SIZE(pkg_state_flag_map); i++) {
		if (sf & pkg_state_flag_map[i].value) {
			strncat(str, pkg_state_flag_map[i].str, len);
			strncat(str, ",", len);
		}
	}

	len = strlen(str);
	str[len-1] = '\0'; /* squash last comma */

	return str;
}

pkg_state_flag_t
pkg_state_flag_from_str(const char *str)
{
     int i;
     int sf = SF_OK;
     const char *sfname;
     unsigned int sfname_len;

     if (strcmp(str, "ok") == 0) {
	  return SF_OK;
     }
     for (i=0; i < ARRAY_SIZE(pkg_state_flag_map); i++) {
	  sfname = pkg_state_flag_map[i].str;
	  sfname_len = strlen(sfname);
	  if (strncmp(str, sfname, sfname_len) == 0) {
	       sf |= pkg_state_flag_map[i].value;
	       str += sfname_len;
	       if (str[0] == ',') {
		    str++;
	       } else {
		    break;
	       }
	  }
     }

     return sf;
}

static const char *
pkg_state_status_to_str(pkg_state_status_t ss)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_status_map); i++) {
	  if (pkg_state_status_map[i].value == ss) {
	       return pkg_state_status_map[i].str;
	  }
     }

     opkg_msg(ERROR, "Internal error: state_status=%d\n", ss);
     return "<STATE_STATUS_UNKNOWN>";
}

pkg_state_status_t
pkg_state_status_from_str(const char *str)
{
     int i;

     for (i=0; i < ARRAY_SIZE(pkg_state_status_map); i++) {
	  if (strcmp(str, pkg_state_status_map[i].str) == 0) {
	       return pkg_state_status_map[i].value;
	  }
     }

     opkg_msg(ERROR, "Internal error: state_status=%s\n", str);
     return SS_NOT_INSTALLED;
}

void
pkg_formatted_field(FILE *fp, pkg_t *pkg, const char *field)
{
     int i, j;
     char *str;
     int depends_count = pkg->pre_depends_count +
			 pkg->depends_count +
			 pkg->recommends_count +
			 pkg->suggests_count;

     if (strlen(field) < PKG_MINIMUM_FIELD_NAME_LEN) {
	  goto UNKNOWN_FMT_FIELD;
     }

     switch (field[0])
     {
     case 'a':
     case 'A':
	  if (strcasecmp(field, "Architecture") == 0) {
	       if (pkg->architecture) {
                   fprintf(fp, "Architecture: %s\n", pkg->architecture);
	       }
	  } else if (strcasecmp(field, "Auto-Installed") == 0) {
		if (pkg->auto_installed)
		    fprintf(fp, "Auto-Installed: yes\n");
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'c':
     case 'C':
	  if (strcasecmp(field, "Conffiles") == 0) {
	       conffile_list_elt_t *iter;

	       if (nv_pair_list_empty(&pkg->conffiles))
		    return;

               fprintf(fp, "Conffiles:\n");
	       for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
		    if (((conffile_t *)iter->data)->name && ((conffile_t *)iter->data)->value) {
                         fprintf(fp, " %s %s\n",
                                 ((conffile_t *)iter->data)->name,
                                 ((conffile_t *)iter->data)->value);
		    }
	       }
	  } else if (strcasecmp(field, "Conflicts") == 0) {
	       struct depend *cdep;
	       if (pkg->conflicts_count) {
                    fprintf(fp, "Conflicts:");
		    for(i = 0; i < pkg->conflicts_count; i++) {
			cdep = pkg->conflicts[i].possibilities[0];
                        fprintf(fp, "%s %s", i == 0 ? "" : ",",
				cdep->pkg->name);
			if (cdep->version) {
				fprintf(fp, " (%s%s)",
					constraint_to_str(cdep->constraint),
					cdep->version);
			}
                    }
                    fprintf(fp, "\n");
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'd':
     case 'D':
	  if (strcasecmp(field, "Depends") == 0) {
	       if (pkg->depends_count) {
                    fprintf(fp, "Depends:");
		    for (j=0, i=0; i<depends_count; i++) {
			if (pkg->depends[i].type != DEPEND)
				continue;
			str = pkg_depend_str(pkg, i);
			fprintf(fp, "%s %s", j == 0 ? "" : ",", str);
			free(str);
			j++;
                    }
		    fprintf(fp, "\n");
	       }
	  } else if (strcasecmp(field, "Description") == 0) {
	       if (pkg->description) {
                   fprintf(fp, "Description: %s\n", pkg->description);
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
          break;
     case 'e':
     case 'E':
	  if (pkg->essential) {
              fprintf(fp, "Essential: yes\n");
	  }
	  break;
     case 'f':
     case 'F':
	  if (pkg->filename) {
              fprintf(fp, "Filename: %s\n", pkg->filename);
	  }
	  break;
     case 'i':
     case 'I':
	  if (strcasecmp(field, "Installed-Size") == 0) {
               fprintf(fp, "Installed-Size: %ld\n", pkg->installed_size);
	  } else if (strcasecmp(field, "Installed-Time") == 0 && pkg->installed_time) {
               fprintf(fp, "Installed-Time: %lu\n", pkg->installed_time);
	  }
	  break;
     case 'm':
     case 'M':
	  if (strcasecmp(field, "Maintainer") == 0) {
	       if (pkg->maintainer) {
                   fprintf(fp, "Maintainer: %s\n", pkg->maintainer);
	       }
	  } else if (strcasecmp(field, "MD5sum") == 0) {
	       if (pkg->md5sum) {
                   fprintf(fp, "MD5Sum: %s\n", pkg->md5sum);
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'p':
     case 'P':
	  if (strcasecmp(field, "Package") == 0) {
               fprintf(fp, "Package: %s\n", pkg->name);
	  } else if (strcasecmp(field, "Priority") == 0) {
               fprintf(fp, "Priority: %s\n", pkg->priority);
	  } else if (strcasecmp(field, "Provides") == 0) {
	       if (pkg->provides_count) {
                  fprintf(fp, "Provides:");
		  for(i = 1; i < pkg->provides_count; i++) {
                      fprintf(fp, "%s %s", i == 1 ? "" : ",",
				      pkg->provides[i]->name);
                  }
                  fprintf(fp, "\n");
               }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 'r':
     case 'R':
	  if (strcasecmp (field, "Replaces") == 0) {
	       if (pkg->replaces_count) {
                    fprintf(fp, "Replaces:");
		    for (i = 0; i < pkg->replaces_count; i++) {
                        fprintf(fp, "%s %s", i == 0 ? "" : ",",
					pkg->replaces[i]->name);
                    }
                    fprintf(fp, "\n");
	       }
	  } else if (strcasecmp (field, "Recommends") == 0) {
	       if (pkg->recommends_count) {
                    fprintf(fp, "Recommends:");
		    for (j=0, i=0; i<depends_count; i++) {
			if (pkg->depends[i].type != RECOMMEND)
				continue;
			str = pkg_depend_str(pkg, i);
			fprintf(fp, "%s %s", j == 0 ? "" : ",", str);
			free(str);
			j++;
                    }
                    fprintf(fp, "\n");
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 's':
     case 'S':
	  if (strcasecmp(field, "Section") == 0) {
	       if (pkg->section) {
                   fprintf(fp, "Section: %s\n", pkg->section);
	       }
#if defined HAVE_SHA256
	  } else if (strcasecmp(field, "SHA256sum") == 0) {
	       if (pkg->sha256sum) {
                   fprintf(fp, "SHA256sum: %s\n", pkg->sha256sum);
	       }
#endif
	  } else if (strcasecmp(field, "Size") == 0) {
	       if (pkg->size) {
                   fprintf(fp, "Size: %ld\n", pkg->size);
	       }
	  } else if (strcasecmp(field, "Source") == 0) {
	       if (pkg->source) {
                   fprintf(fp, "Source: %s\n", pkg->source);
               }
	  } else if (strcasecmp(field, "Status") == 0) {
               char *pflag = pkg_state_flag_to_str(pkg->state_flag);
               fprintf(fp, "Status: %s %s %s\n",
               	               pkg_state_want_to_str(pkg->state_want),
			       pflag,
                               pkg_state_status_to_str(pkg->state_status));
               free(pflag);
	  } else if (strcasecmp(field, "Suggests") == 0) {
	       if (pkg->suggests_count) {
                    fprintf(fp, "Suggests:");
		    for (j=0, i=0; i<depends_count; i++) {
			if (pkg->depends[i].type != SUGGEST)
				continue;
			str = pkg_depend_str(pkg, i);
			fprintf(fp, "%s %s", j == 0 ? "" : ",", str);
			free(str);
			j++;
                    }
                    fprintf(fp, "\n");
	       }
	  } else {
	       goto UNKNOWN_FMT_FIELD;
	  }
	  break;
     case 't':
     case 'T':
	  if (strcasecmp(field, "Tags") == 0) {
	       if (pkg->tags) {
                   fprintf(fp, "Tags: %s\n", pkg->tags);
	       }
	  }
	  break;
     case 'v':
     case 'V':
	  {
	       char *version = pkg_version_str_alloc(pkg);
	       if (version == NULL)
	            return;
               fprintf(fp, "Version: %s\n", version);
	       free(version);
          }
	  break;
     default:
	  goto UNKNOWN_FMT_FIELD;
     }

     return;

UNKNOWN_FMT_FIELD:
     opkg_msg(ERROR, "Internal error: field=%s\n", field);
}

void
pkg_formatted_info(FILE *fp, pkg_t *pkg)
{
	pkg_formatted_field(fp, pkg, "Package");
	pkg_formatted_field(fp, pkg, "Version");
	pkg_formatted_field(fp, pkg, "Depends");
	pkg_formatted_field(fp, pkg, "Recommends");
	pkg_formatted_field(fp, pkg, "Suggests");
	pkg_formatted_field(fp, pkg, "Provides");
	pkg_formatted_field(fp, pkg, "Replaces");
	pkg_formatted_field(fp, pkg, "Conflicts");
	pkg_formatted_field(fp, pkg, "Status");
	pkg_formatted_field(fp, pkg, "Section");
	pkg_formatted_field(fp, pkg, "Essential");
	pkg_formatted_field(fp, pkg, "Architecture");
	pkg_formatted_field(fp, pkg, "Maintainer");
	pkg_formatted_field(fp, pkg, "MD5sum");
	pkg_formatted_field(fp, pkg, "Size");
	pkg_formatted_field(fp, pkg, "Filename");
	pkg_formatted_field(fp, pkg, "Conffiles");
	pkg_formatted_field(fp, pkg, "Source");
	pkg_formatted_field(fp, pkg, "Description");
	pkg_formatted_field(fp, pkg, "Installed-Time");
	pkg_formatted_field(fp, pkg, "Tags");
	fputs("\n", fp);
}

void
pkg_print_status(pkg_t * pkg, FILE * file)
{
     if (pkg == NULL) {
	  return;
     }

     pkg_formatted_field(file, pkg, "Package");
     pkg_formatted_field(file, pkg, "Version");
     pkg_formatted_field(file, pkg, "Depends");
     pkg_formatted_field(file, pkg, "Recommends");
     pkg_formatted_field(file, pkg, "Suggests");
     pkg_formatted_field(file, pkg, "Provides");
     pkg_formatted_field(file, pkg, "Replaces");
     pkg_formatted_field(file, pkg, "Conflicts");
     pkg_formatted_field(file, pkg, "Status");
     pkg_formatted_field(file, pkg, "Essential");
     pkg_formatted_field(file, pkg, "Architecture");
     pkg_formatted_field(file, pkg, "Conffiles");
     pkg_formatted_field(file, pkg, "Installed-Time");
     pkg_formatted_field(file, pkg, "Auto-Installed");
     fputs("\n", file);
}

/*
 * libdpkg - Debian packaging suite library routines
 * vercmp.c - comparison of version numbers
 *
 * Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 */

/* assume ascii; warning: evaluates x multiple times! */
#define order(x) ((x) == '~' ? -1 \
		: isdigit((x)) ? 0 \
		: !(x) ? 0 \
		: isalpha((x)) ? (x) \
		: (x) + 256)

static int
verrevcmp(const char *val, const char *ref) {
  if (!val) val= "";
  if (!ref) ref= "";

  while (*val || *ref) {
    int first_diff= 0;

    while ( (*val && !isdigit(*val)) || (*ref && !isdigit(*ref)) ) {
      int vc= order(*val), rc= order(*ref);
      if (vc != rc) return vc - rc;
      val++; ref++;
    }

    while ( *val == '0' ) val++;
    while ( *ref == '0' ) ref++;
    while (isdigit(*val) && isdigit(*ref)) {
      if (!first_diff) first_diff= *val - *ref;
      val++; ref++;
    }
    if (isdigit(*val)) return 1;
    if (isdigit(*ref)) return -1;
    if (first_diff) return first_diff;
  }
  return 0;
}

int
pkg_compare_versions(const pkg_t *pkg, const pkg_t *ref_pkg)
{
     int r;

     if (pkg->epoch > ref_pkg->epoch) {
	  return 1;
     }

     if (pkg->epoch < ref_pkg->epoch) {
	  return -1;
     }

     r = verrevcmp(pkg->version, ref_pkg->version);
     if (r) {
	  return r;
     }

     r = verrevcmp(pkg->revision, ref_pkg->revision);
     if (r) {
	  return r;
     }

     return r;
}


int
pkg_version_satisfied(pkg_t *it, pkg_t *ref, const char *op)
{
     int r;

     r = pkg_compare_versions(it, ref);

     if (strcmp(op, "<=") == 0 || strcmp(op, "<") == 0) {
	  return r <= 0;
     }

     if (strcmp(op, ">=") == 0 || strcmp(op, ">") == 0) {
	  return r >= 0;
     }

     if (strcmp(op, "<<") == 0) {
	  return r < 0;
     }

     if (strcmp(op, ">>") == 0) {
	  return r > 0;
     }

     if (strcmp(op, "=") == 0) {
	  return r == 0;
     }

     opkg_msg(ERROR, "Unknown operator: %s.\n", op);
     return 0;
}

int
pkg_name_version_and_architecture_compare(const void *p1, const void *p2)
{
     const pkg_t *a = *(const pkg_t**) p1;
     const pkg_t *b = *(const pkg_t**) p2;
     int namecmp;
     int vercmp;
     if (!a->name || !b->name) {
       opkg_msg(ERROR, "Internal error: a->name=%p, b->name=%p.\n",
	       a->name, b->name);
       return 0;
     }

     namecmp = strcmp(a->name, b->name);
     if (namecmp)
	  return namecmp;
     vercmp = pkg_compare_versions(a, b);
     if (vercmp)
	  return vercmp;
     if (!a->arch_priority || !b->arch_priority) {
       opkg_msg(ERROR, "Internal error: a->arch_priority=%i b->arch_priority=%i.\n",
	       a->arch_priority, b->arch_priority);
       return 0;
     }
     if (a->arch_priority > b->arch_priority)
	  return 1;
     if (a->arch_priority < b->arch_priority)
	  return -1;
     return 0;
}

int
abstract_pkg_name_compare(const void *p1, const void *p2)
{
     const abstract_pkg_t *a = *(const abstract_pkg_t **)p1;
     const abstract_pkg_t *b = *(const abstract_pkg_t **)p2;
     if (!a->name || !b->name) {
       opkg_msg(ERROR, "Internal error: a->name=%p b->name=%p.\n",
	       a->name, b->name);
       return 0;
     }
     return strcmp(a->name, b->name);
}


char *
pkg_version_str_alloc(pkg_t *pkg)
{
	char *version;

	if (pkg->epoch) {
		if (pkg->revision)
			sprintf_alloc(&version, "%d:%s-%s",
				pkg->epoch, pkg->version, pkg->revision);
		else
			sprintf_alloc(&version, "%d:%s",
				pkg->epoch, pkg->version);
	} else {
		if (pkg->revision)
			sprintf_alloc(&version, "%s-%s",
				pkg->version, pkg->revision);
		else
			version = xstrdup(pkg->version);
	}

	return version;
}

/*
 * XXX: this should be broken into two functions
 */
str_list_t *
pkg_get_installed_files(pkg_t *pkg)
{
     int err, fd;
     char *list_file_name = NULL;
     FILE *list_file = NULL;
     char *line;
     char *installed_file_name;
     unsigned int rootdirlen = 0;
     int list_from_package;

     pkg->installed_files_ref_cnt++;

     if (pkg->installed_files) {
	  return pkg->installed_files;
     }

     pkg->installed_files = str_list_alloc();

     /*
      * For installed packages, look at the package.list file in the database.
      * For uninstalled packages, get the file list directly from the package.
      */
     if (pkg->state_status == SS_NOT_INSTALLED || pkg->dest == NULL)
	     list_from_package = 1;
     else
	     list_from_package = 0;

     if (list_from_package) {
	  if (pkg->local_filename == NULL) {
	       return pkg->installed_files;
	  }
	  /* XXX: CLEANUP: Maybe rewrite this to avoid using a temporary
	     file. In other words, change deb_extract so that it can
	     simply return the file list as a char *[] rather than
	     insisting on writing it to a FILE * as it does now. */
	  sprintf_alloc(&list_file_name, "%s/%s.list.XXXXXX",
					  conf->tmp_dir, pkg->name);
	  fd = mkstemp(list_file_name);
	  if (fd == -1) {
	       opkg_perror(ERROR, "Failed to make temp file %s.",
			       list_file_name);
	       free(list_file_name);
	       return pkg->installed_files;
	  }
	  list_file = fdopen(fd, "r+");
	  if (list_file == NULL) {
	       opkg_perror(ERROR, "Failed to fdopen temp file %s.",
			       list_file_name);
	       close(fd);
	       unlink(list_file_name);
	       free(list_file_name);
	       return pkg->installed_files;
	  }
	  err = pkg_extract_data_file_names_to_stream(pkg, list_file);
	  if (err) {
	       opkg_msg(ERROR, "Error extracting file list from %s.\n",
			       pkg->local_filename);
	       fclose(list_file);
	       unlink(list_file_name);
	       free(list_file_name);
	       str_list_deinit(pkg->installed_files);
	       pkg->installed_files = NULL;
	       return NULL;
	  }
	  rewind(list_file);
     } else {
	  sprintf_alloc(&list_file_name, "%s/%s.list",
			pkg->dest->info_dir, pkg->name);
	  list_file = fopen(list_file_name, "r");
	  if (list_file == NULL) {
	       opkg_perror(ERROR, "Failed to open %s",
		       list_file_name);
	       free(list_file_name);
	       return pkg->installed_files;
	  }
	  free(list_file_name);
     }

     if (conf->offline_root)
          rootdirlen = strlen(conf->offline_root);

     while (1) {
	  char *file_name;

	  line = file_read_line_alloc(list_file);
	  if (line == NULL) {
	       break;
	  }
	  file_name = line;

	  if (list_from_package) {
	       if (*file_name == '.') {
		    file_name++;
	       }
	       if (*file_name == '/') {
		    file_name++;
	       }
	       sprintf_alloc(&installed_file_name, "%s%s",
			       pkg->dest->root_dir, file_name);
	  } else {
	       if (conf->offline_root &&
	               strncmp(conf->offline_root, file_name, rootdirlen)) {
	            sprintf_alloc(&installed_file_name, "%s%s",
				    conf->offline_root, file_name);
	       } else {
	            // already contains root_dir as header -> ABSOLUTE
	            sprintf_alloc(&installed_file_name, "%s", file_name);
	       }
	  }
	  str_list_append(pkg->installed_files, installed_file_name);
          free(installed_file_name);
	  free(line);
     }

     fclose(list_file);

     if (list_from_package) {
	  unlink(list_file_name);
	  free(list_file_name);
     }

     return pkg->installed_files;
}

/* XXX: CLEANUP: This function and it's counterpart,
   (pkg_get_installed_files), do not match our init/deinit naming
   convention. Nor the alloc/free convention. But, then again, neither
   of these conventions currrently fit the way these two functions
   work. */
void
pkg_free_installed_files(pkg_t *pkg)
{
     pkg->installed_files_ref_cnt--;

     if (pkg->installed_files_ref_cnt > 0)
	  return;

     if (pkg->installed_files) {
         str_list_purge(pkg->installed_files);
     }

     pkg->installed_files = NULL;
}

void
pkg_remove_installed_files_list(pkg_t *pkg)
{
	char *list_file_name;

	sprintf_alloc(&list_file_name, "%s/%s.list",
		pkg->dest->info_dir, pkg->name);

	if (!conf->noaction)
		(void)unlink(list_file_name);

	free(list_file_name);
}

conffile_t *
pkg_get_conffile(pkg_t *pkg, const char *file_name)
{
     conffile_list_elt_t *iter;
     conffile_t *conffile;

     if (pkg == NULL) {
	  return NULL;
     }

     for (iter = nv_pair_list_first(&pkg->conffiles); iter; iter = nv_pair_list_next(&pkg->conffiles, iter)) {
	  conffile = (conffile_t *)iter->data;

	  if (strcmp(conffile->name, file_name) == 0) {
	       return conffile;
	  }
     }

     return NULL;
}

int
pkg_run_script(pkg_t *pkg, const char *script, const char *args)
{
     int err;
     char *path;
     char *cmd;

     if (conf->noaction)
	     return 0;

     /* XXX: FEATURE: When conf->offline_root is set, we should run the
	maintainer script within a chroot environment. */
     if (conf->offline_root && !conf->force_postinstall) {
          opkg_msg(INFO, "Offline root mode: not running %s.%s.\n",
			  pkg->name, script);
	  return 0;
     }

     /* Installed packages have scripts in pkg->dest->info_dir, uninstalled packages
	have scripts in pkg->tmp_unpack_dir. */
     if (pkg->state_status == SS_INSTALLED || pkg->state_status == SS_UNPACKED) {
	  if (pkg->dest == NULL) {
	       opkg_msg(ERROR, "Internal error: %s has a NULL dest.\n",
		       pkg->name);
	       return -1;
	  }
	  sprintf_alloc(&path, "%s/%s.%s", pkg->dest->info_dir, pkg->name, script);
     } else {
	  if (pkg->tmp_unpack_dir == NULL) {
	       opkg_msg(ERROR, "Internal error: %s has a NULL tmp_unpack_dir.\n",
		       pkg->name);
	       return -1;
	  }
	  sprintf_alloc(&path, "%s/%s", pkg->tmp_unpack_dir, script);
     }

     opkg_msg(INFO, "Running script %s.\n", path);

     setenv("PKG_ROOT",
	    pkg->dest ? pkg->dest->root_dir : conf->default_dest->root_dir, 1);

     if (! file_exists(path)) {
	  free(path);
	  return 0;
     }

     sprintf_alloc(&cmd, "%s %s", path, args);
     free(path);
     {
	  const char *argv[] = {"sh", "-c", cmd, NULL};
	  err = xsystem(argv);
     }
     free(cmd);

     if (err) {
	  opkg_msg(ERROR, "package \"%s\" %s script returned status %d.\n", 
               pkg->name, script, err);
	  return err;
     }

     return 0;
}

int
pkg_arch_supported(pkg_t *pkg)
{
     nv_pair_list_elt_t *l;

     if (!pkg->architecture)
	  return 1;

     list_for_each_entry(l , &conf->arch_list.head, node) {
	  nv_pair_t *nv = (nv_pair_t *)l->data;
	  if (strcmp(nv->name, pkg->architecture) == 0) {
	       opkg_msg(DEBUG, "Arch %s (priority %s) supported for pkg %s.\n",
			       nv->name, nv->value, pkg->name);
	       return 1;
	  }
     }

     opkg_msg(DEBUG, "Arch %s unsupported for pkg %s.\n",
		     pkg->architecture, pkg->name);
     return 0;
}

void
pkg_info_preinstall_check(void)
{
     int i;
     pkg_vec_t *installed_pkgs = pkg_vec_alloc();

     /* update the file owner data structure */
     opkg_msg(INFO, "Updating file owner list.\n");
     pkg_hash_fetch_all_installed(installed_pkgs);
     for (i = 0; i < installed_pkgs->len; i++) {
	  pkg_t *pkg = installed_pkgs->pkgs[i];
	  str_list_t *installed_files = pkg_get_installed_files(pkg); /* this causes installed_files to be cached */
	  str_list_elt_t *iter, *niter;
	  if (installed_files == NULL) {
	       opkg_msg(ERROR, "Failed to determine installed "
			       "files for pkg %s.\n", pkg->name);
	       break;
	  }
	  for (iter = str_list_first(installed_files), niter = str_list_next(installed_files, iter);
                  iter;
                  iter = niter, niter = str_list_next(installed_files, iter)) {
	       char *installed_file = (char *) iter->data;
	       file_hash_set_file_owner(installed_file, pkg);
	  }
	  pkg_free_installed_files(pkg);
     }
     pkg_vec_free(installed_pkgs);
}

struct pkg_write_filelist_data {
     pkg_t *pkg;
     FILE *stream;
};

static void
pkg_write_filelist_helper(const char *key, void *entry_, void *data_)
{
     struct pkg_write_filelist_data *data = data_;
     pkg_t *entry = entry_;
     if (entry == data->pkg) {
	  fprintf(data->stream, "%s\n", key);
     }
}

int
pkg_write_filelist(pkg_t *pkg)
{
	struct pkg_write_filelist_data data;
	char *list_file_name;

	sprintf_alloc(&list_file_name, "%s/%s.list",
			pkg->dest->info_dir, pkg->name);

	opkg_msg(INFO, "Creating %s file for pkg %s.\n",
			list_file_name, pkg->name);

	data.stream = fopen(list_file_name, "w");
	if (!data.stream) {
		opkg_perror(ERROR, "Failed to open %s",
			list_file_name);
		free(list_file_name);
		return -1;
	}

	data.pkg = pkg;
	hash_table_foreach(&conf->file_hash, pkg_write_filelist_helper, &data);
	fclose(data.stream);
	free(list_file_name);

	pkg->state_flag &= ~SF_FILELIST_CHANGED;

	return 0;
}

int
pkg_write_changed_filelists(void)
{
	pkg_vec_t *installed_pkgs = pkg_vec_alloc();
	int i, err, ret = 0;

	if (conf->noaction)
		return 0;

	opkg_msg(INFO, "Saving changed filelists.\n");

	pkg_hash_fetch_all_installed(installed_pkgs);
	for (i = 0; i < installed_pkgs->len; i++) {
		pkg_t *pkg = installed_pkgs->pkgs[i];
		if (pkg->state_flag & SF_FILELIST_CHANGED) {
			err = pkg_write_filelist(pkg);
			if (err)
				ret = -1;
		}
	}

	pkg_vec_free (installed_pkgs);

	return ret;
}
