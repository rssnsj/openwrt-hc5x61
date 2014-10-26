/* vi: set sw=4 ts=4: */
/*
 * Busybox main internal header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef	__LIBBB_H__
#define	__LIBBB_H__    1

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netdb.h>

#include "../libopkg/opkg_message.h"

#ifndef FALSE
#define FALSE   ((int) 0)
#endif

#ifndef TRUE
#define TRUE    ((int) 1)
#endif

#define error_msg(fmt, args...) opkg_msg(ERROR, fmt"\n", ##args)
#define perror_msg(fmt, args...) opkg_perror(ERROR, fmt, ##args)
#define error_msg_and_die(fmt, args...) \
	do { \
		error_msg(fmt, ##args); \
		exit(EXIT_FAILURE); \
	} while (0)
#define perror_msg_and_die(fmt, args...) \
	do { \
		perror_msg(fmt, ##args); \
		exit(EXIT_FAILURE); \
	} while (0)

extern void archive_xread_all(int fd, char *buf, size_t count);

const char *mode_string(int mode);
const char *time_string(time_t timeVal);

int copy_file(const char *source, const char *dest, int flags);
int copy_file_chunk(FILE *src_file, FILE *dst_file, unsigned long long chunksize);
ssize_t safe_read(int fd, void *buf, size_t count);
ssize_t full_read(int fd, char *buf, int len);

extern int parse_mode( const char* s, mode_t* theMode);

extern FILE *wfopen(const char *path, const char *mode);
extern FILE *xfopen(const char *path, const char *mode);

extern void *xmalloc (size_t size);
extern void *xrealloc(void *old, size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup (const char *s);
extern char *xstrndup (const char *s, int n);
extern char *safe_strncpy(char *dst, const char *src, size_t size);

char *xreadlink(const char *path);
char *concat_path_file(const char *path, const char *filename);
char *last_char_is(const char *s, int c);

typedef struct file_headers_s {
	char *name;
	char *link_name;
	off_t size;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	time_t mtime;
	dev_t device;
} file_header_t;

enum extract_functions_e {
	extract_verbose_list = 1,
	extract_list = 2,
	extract_one_to_buffer = 4,
	extract_to_stream = 8,
	extract_all_to_fs = 16,
	extract_preserve_date = 32,
	extract_data_tar_gz = 64,
	extract_control_tar_gz = 128,
	extract_unzip_only = 256,
	extract_unconditional = 512,
	extract_create_leading_dirs = 1024,
	extract_quiet = 2048,
	extract_exclude_list = 4096
};

char *deb_extract(const char *package_filename, FILE *out_stream,
		const int extract_function, const char *prefix,
		const char *filename, int *err);

extern int unzip(FILE *l_in_file, FILE *l_out_file);
extern int gz_close(int gunzip_pid);
extern FILE *gz_open(FILE *compressed_file, int *pid);

int make_directory (const char *path, long mode, int flags);

enum {
	FILEUTILS_PRESERVE_STATUS = 1,
	FILEUTILS_PRESERVE_SYMLINKS = 2,
	FILEUTILS_RECUR = 4,
	FILEUTILS_FORCE = 8,
};

#endif /* __LIBBB_H__ */
