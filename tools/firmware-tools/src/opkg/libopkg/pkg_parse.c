/* pkg_parse.c - the opkg package management system

   Copyright (C) 2009 Ubiq Technologies <graham.gower@gmail.com>

   Steven M. Ayer
   Copyright (C) 2002 Compaq Computer Corporation

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
#include <ctype.h>

#include "pkg.h"
#include "opkg_utils.h"
#include "pkg_parse.h"
#include "libbb/libbb.h"

#include "parse_util.h"

static void
parse_status(pkg_t *pkg, const char *sstr)
{
	char sw_str[64], sf_str[64], ss_str[64];

	if (sscanf(sstr, "Status: %63s %63s %63s",
				sw_str, sf_str, ss_str) != 3) {
		opkg_msg(ERROR, "Failed to parse Status line for %s\n",
				pkg->name);
		return;
	}

	pkg->state_want = pkg_state_want_from_str(sw_str);
	pkg->state_flag = pkg_state_flag_from_str(sf_str);
	pkg->state_status = pkg_state_status_from_str(ss_str);
}

static void
parse_conffiles(pkg_t *pkg, const char *cstr)
{
	char file_name[1024], md5sum[35];

	if (sscanf(cstr, "%1023s %34s", file_name, md5sum) != 2) {
		opkg_msg(ERROR, "Failed to parse Conffiles line for %s\n",
				pkg->name);
		return;
	}

	conffile_list_append(&pkg->conffiles, file_name, md5sum);
}

int
parse_version(pkg_t *pkg, const char *vstr)
{
	char *colon;

	if (strncmp(vstr, "Version:", 8) == 0)
		vstr += 8;

	while (*vstr && isspace(*vstr))
		vstr++;

	colon = strchr(vstr, ':');
	if (colon) {
		errno = 0;
		pkg->epoch = strtoul(vstr, NULL, 10);
		if (errno) {
			opkg_perror(ERROR, "%s: invalid epoch", pkg->name);
		}
		vstr = ++colon;
	} else {
		pkg->epoch= 0;
	}

	pkg->version = xstrdup(vstr);
	pkg->revision = strrchr(pkg->version,'-');

	if (pkg->revision)
		*pkg->revision++ = '\0';

	return 0;
}

static int
get_arch_priority(const char *arch)
{
	nv_pair_list_elt_t *l;

	list_for_each_entry(l , &conf->arch_list.head, node) {
		nv_pair_t *nv = (nv_pair_t *)l->data;
		if (strcmp(nv->name, arch) == 0)
			return strtol(nv->value, NULL, 0);
	}
	return 0;
}

int
pkg_parse_line(void *ptr, const char *line, uint mask)
{
	pkg_t *pkg = (pkg_t *) ptr;

	/* these flags are a bit hackish... */
	static int reading_conffiles = 0, reading_description = 0;
	int ret = 0;

	/* Exclude globally masked fields. */
	mask |= conf->pfm;

	/* Flip the semantics of the mask. */
	mask ^= PFM_ALL;

	switch (*line) {
	case 'A':
		if ((mask & PFM_ARCHITECTURE ) && is_field("Architecture", line)) {
			pkg->architecture = parse_simple("Architecture", line);
			pkg->arch_priority = get_arch_priority(pkg->architecture);
		} else if ((mask & PFM_AUTO_INSTALLED) && is_field("Auto-Installed", line)) {
			char *tmp = parse_simple("Auto-Installed", line);
			if (strcmp(tmp, "yes") == 0)
			    pkg->auto_installed = 1;
			free(tmp);
		}
		break;

	case 'C':
		if ((mask & PFM_CONFFILES) && is_field("Conffiles", line)) {
			reading_conffiles = 1;
			reading_description = 0;
			goto dont_reset_flags;
	    	}
		else if ((mask & PFM_CONFLICTS) && is_field("Conflicts", line))
			pkg->conflicts_str = parse_list(line, &pkg->conflicts_count, ',', 0);
		break;

	case 'D':
		if ((mask & PFM_DESCRIPTION) && is_field("Description", line)) {
			pkg->description = parse_simple("Description", line);
			reading_conffiles = 0;
			reading_description = 1;
			goto dont_reset_flags;
		} else if ((mask & PFM_DEPENDS) && is_field("Depends", line))
			pkg->depends_str = parse_list(line, &pkg->depends_count, ',', 0);
		break;

	case 'E':
		if((mask & PFM_ESSENTIAL) && is_field("Essential", line)) {
			char *tmp = parse_simple("Essential", line);
			if (strcmp(tmp, "yes") == 0)
				pkg->essential = 1;
			free(tmp);
		}
		break;

	case 'F':
		if((mask & PFM_FILENAME) && is_field("Filename", line))
			pkg->filename = parse_simple("Filename", line);
		break;

	case 'I':
		if ((mask & PFM_INSTALLED_SIZE) && is_field("Installed-Size", line)) {
			char *tmp = parse_simple("Installed-Size", line);
			pkg->installed_size = strtoul(tmp, NULL, 0);
			free (tmp);
		} else if ((mask & PFM_INSTALLED_TIME) && is_field("Installed-Time", line)) {
			char *tmp = parse_simple("Installed-Time", line);
			pkg->installed_time = strtoul(tmp, NULL, 0);
			free (tmp);
		}
		break;

	case 'M':
		if ((mask & PFM_MD5SUM) && is_field("MD5sum:", line))
			pkg->md5sum = parse_simple("MD5sum", line);
			/* The old opkg wrote out status files with the wrong
			* case for MD5sum, let's parse it either way */
		else if ((mask & PFM_MD5SUM) && is_field("MD5Sum:", line)) 
			pkg->md5sum = parse_simple("MD5Sum", line);
		else if((mask & PFM_MAINTAINER) && is_field("Maintainer", line))
			pkg->maintainer = parse_simple("Maintainer", line);
		break;

	case 'P':
		if ((mask & PFM_PACKAGE) && is_field("Package", line))
			pkg->name = parse_simple("Package", line);
		else if ((mask & PFM_PRIORITY) && is_field("Priority", line))
			pkg->priority = parse_simple("Priority", line);
		else if ((mask & PFM_PROVIDES) && is_field("Provides", line))
			pkg->provides_str = parse_list(line, &pkg->provides_count, ',', 0);
		else if ((mask & PFM_PRE_DEPENDS) && is_field("Pre-Depends", line))
			pkg->pre_depends_str = parse_list(line, &pkg->pre_depends_count, ',', 0);
		break;

	case 'R':
		if ((mask & PFM_RECOMMENDS) && is_field("Recommends", line))
			pkg->recommends_str = parse_list(line, &pkg->recommends_count, ',', 0);
		else if ((mask & PFM_REPLACES) && is_field("Replaces", line))
			pkg->replaces_str = parse_list(line, &pkg->replaces_count, ',', 0);

		break;

	case 'S':
		if ((mask & PFM_SECTION) && is_field("Section", line))
			pkg->section = parse_simple("Section", line);
#ifdef HAVE_SHA256
		else if ((mask & PFM_SHA256SUM) && is_field("SHA256sum", line))
			pkg->sha256sum = parse_simple("SHA256sum", line);
#endif
		else if ((mask & PFM_SIZE) && is_field("Size", line)) {
			char *tmp = parse_simple("Size", line);
			pkg->size = strtoul(tmp, NULL, 0);
			free (tmp);
		} else if ((mask & PFM_SOURCE) && is_field("Source", line))
			pkg->source = parse_simple("Source", line);
		else if ((mask & PFM_STATUS) && is_field("Status", line))
			parse_status(pkg, line);
		else if ((mask & PFM_SUGGESTS) && is_field("Suggests", line))
			pkg->suggests_str = parse_list(line, &pkg->suggests_count, ',', 0);
		break;

	case 'T':
		if ((mask & PFM_TAGS) && is_field("Tags", line))
			pkg->tags = parse_simple("Tags", line);
		break;

	case 'V':
		if ((mask & PFM_VERSION) && is_field("Version", line))
			parse_version(pkg, line);
		break;

	case ' ':
		if ((mask & PFM_DESCRIPTION) && reading_description) {
			pkg->description = xrealloc(pkg->description,
						strlen(pkg->description)
						+ 1 + strlen(line) + 1);
			strcat(pkg->description, "\n");
			strcat(pkg->description, (line));
			goto dont_reset_flags;
		} else if ((mask & PFM_CONFFILES) && reading_conffiles) {
			parse_conffiles(pkg, line);
			goto dont_reset_flags;
		}

		/* FALLTHROUGH */
	default:
		/* For package lists, signifies end of package. */
		if(line_is_blank(line)) {
			ret = 1;
			break;
		}
	}

	reading_description = 0;
	reading_conffiles = 0;

dont_reset_flags:

	return ret;
}

int
pkg_parse_from_stream(pkg_t *pkg, FILE *fp, uint mask)
{
	int ret;
	char *buf;
	const size_t len = 4096;

	buf = xmalloc(len);
	ret = parse_from_stream_nomalloc(pkg_parse_line, pkg, fp, mask, &buf, len);
	free(buf);

	if (pkg->name == NULL) {
		/* probably just a blank line */
		ret = 1;
	}

	return ret;
}
