/* parse_util.c - the opkg package management system

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

#include <ctype.h>

#include "opkg_utils.h"
#include "libbb/libbb.h"

#include "parse_util.h"
#include "pkg_parse.h"

int
is_field(const char *type, const char *line)
{
	if (!strncmp(line, type, strlen(type)))
		return 1;
	return 0;
}

char *
parse_simple(const char *type, const char *line)
{
	return trim_xstrdup(line + strlen(type) + 1);
}

/*
 * Parse a comma separated string into an array.
 */
char **
parse_list(const char *raw, unsigned int *count, const char sep, int skip_field)
{
	char **depends = NULL;
	const char *start, *end;
	int line_count = 0;

	/* skip past the "Field:" marker */
	if (!skip_field) {
	while (*raw && *raw != ':')
		raw++;
	raw++;
	}

	if (line_is_blank(raw)) {
		*count = line_count;
		return NULL;
	}

	while (*raw) {
		depends = xrealloc(depends, sizeof(char *) * (line_count + 1));
	
		while (isspace(*raw))
			raw++;

		start = raw;
		while (*raw != sep && *raw)
			raw++;
		end = raw;

		while (end > start && isspace(*end))
			end--;

		if (sep == ' ')
			end++;

		depends[line_count] = xstrndup(start, end-start);

        	line_count++;
		if (*raw == sep)
		    raw++;
	}

	*count = line_count;
	return depends;
}

int
parse_from_stream_nomalloc(parse_line_t parse_line, void *item, FILE *fp, uint mask,
						char **buf0, size_t buf0len)
{
	int ret, lineno;
	char *buf, *nl;
	size_t buflen;

	lineno = 1;
	ret = 0;

	buflen = buf0len;
	buf = *buf0;
	buf[0] = '\0';

	while (1) {
		if (fgets(buf, (int)buflen, fp) == NULL) {
			if (ferror(fp)) {
				opkg_perror(ERROR, "fgets");
				ret = -1;
			} else if (strlen(*buf0) == buf0len-1) {
				opkg_msg(ERROR, "Missing new line character"
						" at end of file!\n");
				parse_line(item, *buf0, mask);
			}
			break;
		}

		nl = strchr(buf, '\n');
		if (nl == NULL) {
			if (strlen(buf) < buflen-1) {
				/*
				 * Line could be exactly buflen-1 long and
				 * missing a newline, but we won't know until
				 * fgets fails to read more data.
				 */
				opkg_msg(ERROR, "Missing new line character"
						" at end of file!\n");
				parse_line(item, *buf0, mask);
				break;
			}
			if (buf0len >= EXCESSIVE_LINE_LEN) {
				opkg_msg(ERROR, "Excessively long line at "
					"%d. Corrupt file?\n",
					lineno);
				ret = -1;
				break;
			}

			/*
			 * Realloc and point buf past the data already read,
			 * at the NULL terminator inserted by fgets.
			 * |<--------------- buf0len ----------------->|
			 * |                     |<------- buflen ---->|
			 * |---------------------|---------------------|
			 * buf0                   buf
			 */
			buflen = buf0len +1;
			buf0len *= 2;
			*buf0 = xrealloc(*buf0, buf0len);
			buf = *buf0 + buflen -2;

			continue;
		}

		*nl = '\0';

		lineno++;

		if (parse_line(item, *buf0, mask))
			break;

		buf = *buf0;
		buflen = buf0len;
		buf[0] = '\0';
	}

	return ret;
}

