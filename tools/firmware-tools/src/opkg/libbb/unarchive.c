/*
 *  Copyright (C) 2000 by Glenn McGrath
 *  Copyright (C) 2001 by Laurence Anderson
 *
 *  Based on previous work by busybox developers and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <libgen.h>

#include "libbb.h"

#define CONFIG_FEATURE_TAR_OLDGNU_COMPATABILITY 1
#define CONFIG_FEATURE_TAR_GNU_EXTENSIONS

#ifdef CONFIG_FEATURE_TAR_GNU_EXTENSIONS
static char *longname = NULL;
static char *linkname = NULL;
#endif

off_t archive_offset;

#define SEEK_BUF 4096
static ssize_t
seek_by_read(FILE* fd, size_t len)
{
        ssize_t cc, total = 0;
        char buf[SEEK_BUF];

        while (len) {
                cc = fread(buf, sizeof(buf[0]),
                                len > SEEK_BUF ? SEEK_BUF : len,
                                fd);

                total += cc;
                len -= cc;

                if(feof(fd) || ferror(fd))
                        break;
        }
        return total;
}

static void
seek_sub_file(FILE *fd, const int count)
{
	archive_offset += count;

	/* Do not use fseek() on a pipe. It may fail with ESPIPE, leaving the
	 * stream at an undefined location.
	 */
        seek_by_read(fd, count);

	return;
}


/* Extract the data postioned at src_stream to either filesystem, stdout or
 * buffer depending on the value of 'function' which is defined in libbb.h
 *
 * prefix doesnt have to be just a directory, it may prefix the filename as well.
 *
 * e.g. '/var/lib/dpkg/info/dpkg.' will extract all files to the base bath
 * '/var/lib/dpkg/info/' and all files/dirs created in that dir will have
 * 'dpkg.' as their prefix
 *
 * For this reason if prefix does point to a dir then it must end with a
 * trailing '/' or else the last dir will be assumed to be the file prefix
 */
static char *
extract_archive(FILE *src_stream, FILE *out_stream,
		const file_header_t *file_entry, const int function,
		const char *prefix,
		int *err)
{
	FILE *dst_stream = NULL;
	char *full_name = NULL;
	char *full_link_name = NULL;
	char *buffer = NULL;
	struct utimbuf t;

	*err = 0;

	/* prefix doesnt have to be a proper path it may prepend
	 * the filename as well */
	if (prefix != NULL) {
		/* strip leading '/' in filename to extract as prefix may not be dir */
		/* Cant use concat_path_file here as prefix might not be a directory */
		char *path = file_entry->name;
		if (strncmp("./", path, 2) == 0) {
			path += 2;
			if (strlen(path) == 0)
				/* Do nothing, current dir already exists. */
				return NULL;
		}
		full_name = xmalloc(strlen(prefix) + strlen(path) + 1);
		strcpy(full_name, prefix);
		strcat(full_name, path);
                if ( file_entry->link_name ){
		   full_link_name = xmalloc(strlen(prefix) + strlen(file_entry->link_name) + 1);
		   strcpy(full_link_name, prefix);
		   strcat(full_link_name, file_entry->link_name);
                }
	} else {
		full_name = xstrdup(file_entry->name);
                if ( file_entry->link_name )
		   full_link_name = xstrdup(file_entry->link_name);
	}


	if (function & extract_to_stream) {
		if (S_ISREG(file_entry->mode)) {
			*err = copy_file_chunk(src_stream, out_stream, file_entry->size);
			archive_offset += file_entry->size;
		}
	}
	else if (function & extract_one_to_buffer) {
		if (S_ISREG(file_entry->mode)) {
			buffer = (char *) xmalloc(file_entry->size + 1);
			fread(buffer, 1, file_entry->size, src_stream);
			buffer[file_entry->size] = '\0';
			archive_offset += file_entry->size;
			goto cleanup;
		}
	}
	else if (function & extract_all_to_fs) {
		struct stat oldfile;
		int stat_res;
		stat_res = lstat (full_name, &oldfile);
		if (stat_res == 0) { /* The file already exists */
			if ((function & extract_unconditional) || (oldfile.st_mtime < file_entry->mtime)) {
				if (!S_ISDIR(oldfile.st_mode)) {
					unlink(full_name); /* Directories might not be empty etc */
				}
			} else {
				if ((function & extract_quiet) != extract_quiet) {
					*err = -1;
					error_msg("%s not created: newer or same age file exists", file_entry->name);
				}
				seek_sub_file(src_stream, file_entry->size);
				goto cleanup;
			}
		}
		if (function & extract_create_leading_dirs) { /* Create leading directories with default umask */
			char *buf, *parent;
			buf = xstrdup(full_name);
			parent = dirname(buf);
			if (make_directory (parent, -1, FILEUTILS_RECUR) != 0) {
				if ((function & extract_quiet) != extract_quiet) {
					*err = -1;
					error_msg("couldn't create leading directories");
				}
			}
			free (buf);
		}
		switch(file_entry->mode & S_IFMT) {
			case S_IFREG:
				if (file_entry->link_name) { /* Found a cpio hard link */
					if (link(full_link_name, full_name) != 0) {
						if ((function & extract_quiet) != extract_quiet) {
							*err = -1;
							perror_msg("Cannot link from %s to '%s'",
								file_entry->name, file_entry->link_name);
						}
					}
				} else {
					if ((dst_stream = wfopen(full_name, "w")) == NULL) {
						*err = -1;
						seek_sub_file(src_stream, file_entry->size);
						goto cleanup;
					}
					archive_offset += file_entry->size;
					*err = copy_file_chunk(src_stream, dst_stream, file_entry->size);
					fclose(dst_stream);
				}
				break;
			case S_IFDIR:
				if (stat_res != 0) {
					if (mkdir(full_name, file_entry->mode) < 0) {
						if ((function & extract_quiet) != extract_quiet) {
							*err = -1;
							perror_msg("Cannot make dir %s", full_name);
						}
					}
				}
				break;
			case S_IFLNK:
				if (symlink(file_entry->link_name, full_name) < 0) {
					if ((function & extract_quiet) != extract_quiet) {
						*err = -1;
						perror_msg("Cannot create symlink from %s to '%s'", file_entry->name, file_entry->link_name);
					}
					goto cleanup;
				}
				break;
			case S_IFSOCK:
			case S_IFBLK:
			case S_IFCHR:
			case S_IFIFO:
				if (mknod(full_name, file_entry->mode, file_entry->device) == -1) {
					if ((function & extract_quiet) != extract_quiet) {
						*err = -1;
						perror_msg("Cannot create node %s", file_entry->name);
					}
					goto cleanup;
				}
				break;
                         default:
				*err = -1;
				perror_msg("Don't know how to handle %s", full_name);

		}

		/* Changing a symlink's properties normally changes the properties of the
		 * file pointed to, so dont try and change the date or mode, lchown does
		 * does the right thing, but isnt available in older versions of libc */
		if (S_ISLNK(file_entry->mode)) {
#if (__GLIBC__ > 2) && (__GLIBC_MINOR__ > 1)
			lchown(full_name, file_entry->uid, file_entry->gid);
#endif
		} else {
			if (function & extract_preserve_date) {
				t.actime = file_entry->mtime;
				t.modtime = file_entry->mtime;
				utime(full_name, &t);
			}
			chown(full_name, file_entry->uid, file_entry->gid);
			chmod(full_name, file_entry->mode);
		}
	} else {
		/* If we arent extracting data we have to skip it,
		 * if data size is 0 then then just do it anyway
		 * (saves testing for it) */
		seek_sub_file(src_stream, file_entry->size);
	}

	/* extract_list and extract_verbose_list can be used in conjunction
	 * with one of the above four extraction functions, so do this seperately */
	if (function & extract_verbose_list) {
		fprintf(out_stream, "%s %d/%d %8d %s ", mode_string(file_entry->mode),
			file_entry->uid, file_entry->gid,
			(int) file_entry->size, time_string(file_entry->mtime));
	}
	if ((function & extract_list) || (function & extract_verbose_list)){
		/* fputs doesnt add a trailing \n, so use fprintf */
		fprintf(out_stream, "%s\n", file_entry->name);
	}

cleanup:
	free(full_name);
        if ( full_link_name )
	    free(full_link_name);

	return buffer;
}

static char *
unarchive(FILE *src_stream, FILE *out_stream,
		file_header_t *(*get_headers)(FILE *),
		void (*free_headers)(file_header_t *),
		const int extract_function,
		const char *prefix,
		const char **extract_names,
		int *err)
{
	file_header_t *file_entry;
	int extract_flag;
	int i;
	char *buffer = NULL;

	*err = 0;

	archive_offset = 0;
	while ((file_entry = get_headers(src_stream)) != NULL) {
		extract_flag = TRUE;

		if (extract_names != NULL) {
			int found_flag = FALSE;
			char *p = file_entry->name;

			if (p[0] == '.' && p[1] == '/')
				p += 2;

			for(i = 0; extract_names[i] != 0; i++) {
				if (strcmp(extract_names[i], p) == 0) {
					found_flag = TRUE;
					break;
				}
			}
			if (extract_function & extract_exclude_list) {
				if (found_flag == TRUE) {
					extract_flag = FALSE;
				}
			} else {
				/* If its not found in the include list dont extract it */
				if (found_flag == FALSE) {
					extract_flag = FALSE;
				}
			}
		}

		if (extract_flag == TRUE) {
			buffer = extract_archive(src_stream, out_stream,
					file_entry, extract_function,
					prefix, err);
			*err = 0; /* XXX: ignore extraction errors */
			if (*err) {
				free_headers(file_entry);
				break;
			}
		} else {
			/* seek past the data entry */
			seek_sub_file(src_stream, file_entry->size);
		}
		free_headers(file_entry);
	}

	return buffer;
}

static file_header_t *
get_header_ar(FILE *src_stream)
{
	file_header_t *typed;
	union {
		char raw[60];
	 	struct {
 			char name[16];
 			char date[12];
 			char uid[6];
 			char gid[6];
 			char mode[8];
 			char size[10];
 			char magic[2];
 		} formated;
	} ar;
	static char *ar_long_names;

	if (fread(ar.raw, 1, 60, src_stream) != 60) {
		return(NULL);
	}
	archive_offset += 60;
	/* align the headers based on the header magic */
	if ((ar.formated.magic[0] != '`') || (ar.formated.magic[1] != '\n')) {
		/* some version of ar, have an extra '\n' after each data entry,
		 * this puts the next header out by 1 */
		if (ar.formated.magic[1] != '`') {
			error_msg("Invalid magic");
			return(NULL);
		}
		/* read the next char out of what would be the data section,
		 * if its a '\n' then it is a valid header offset by 1*/
		archive_offset++;
		if (fgetc(src_stream) != '\n') {
			error_msg("Invalid magic");
			return(NULL);
		}
		/* fix up the header, we started reading 1 byte too early */
		/* raw_header[60] wont be '\n' as it should, but it doesnt matter */
		memmove(ar.raw, &ar.raw[1], 59);
	}

	typed = (file_header_t *) xcalloc(1, sizeof(file_header_t));

	typed->size = (size_t) atoi(ar.formated.size);
	/* long filenames have '/' as the first character */
	if (ar.formated.name[0] == '/') {
		if (ar.formated.name[1] == '/') {
			/* If the second char is a '/' then this entries data section
			 * stores long filename for multiple entries, they are stored
			 * in static variable long_names for use in future entries */
			ar_long_names = (char *) xrealloc(ar_long_names, typed->size);
			fread(ar_long_names, 1, typed->size, src_stream);
			archive_offset += typed->size;
			/* This ar entries data section only contained filenames for other records
			 * they are stored in the static ar_long_names for future reference */
			return (get_header_ar(src_stream)); /* Return next header */
		} else if (ar.formated.name[1] == ' ') {
			/* This is the index of symbols in the file for compilers */
			seek_sub_file(src_stream, typed->size);
			return (get_header_ar(src_stream)); /* Return next header */
		} else {
			/* The number after the '/' indicates the offset in the ar data section
			(saved in variable long_name) that conatains the real filename */
			if (!ar_long_names) {
				error_msg("Cannot resolve long file name");
				return (NULL);
			}
			typed->name = xstrdup(ar_long_names + atoi(&ar.formated.name[1]));
		}
	} else {
		/* short filenames */
		typed->name = xcalloc(1, 16);
		strncpy(typed->name, ar.formated.name, 16);
	}
	typed->name[strcspn(typed->name, " /")]='\0';

	/* convert the rest of the now valid char header to its typed struct */
	parse_mode(ar.formated.mode, &typed->mode);
	typed->mtime = atoi(ar.formated.date);
	typed->uid = atoi(ar.formated.uid);
	typed->gid = atoi(ar.formated.gid);

	return(typed);
}

static void
free_header_ar(file_header_t *ar_entry)
{
	if (ar_entry == NULL)
		return;

	free(ar_entry->name);
	if (ar_entry->link_name)
		free(ar_entry->link_name);

	free(ar_entry);
}


static file_header_t *
get_header_tar(FILE *tar_stream)
{
	union {
		unsigned char raw[512];
		struct {
			char name[100];		/*   0-99 */
			char mode[8];		/* 100-107 */
			char uid[8];		/* 108-115 */
			char gid[8];		/* 116-123 */
			char size[12];		/* 124-135 */
			char mtime[12];		/* 136-147 */
			char chksum[8];		/* 148-155 */
			char typeflag;		/* 156-156 */
			char linkname[100];	/* 157-256 */
			char magic[6];		/* 257-262 */
			char version[2];	/* 263-264 */
			char uname[32];		/* 265-296 */
			char gname[32];		/* 297-328 */
			char devmajor[8];	/* 329-336 */
			char devminor[8];	/* 337-344 */
			char prefix[155];	/* 345-499 */
			char padding[12];	/* 500-512 */
		} formated;
	} tar;
	file_header_t *tar_entry = NULL;
	long i;
	long sum = 0;

	if (archive_offset % 512 != 0) {
		seek_sub_file(tar_stream, 512 - (archive_offset % 512));
	}

	if (fread(tar.raw, 1, 512, tar_stream) != 512) {
		/* Unfortunately its common for tar files to have all sorts of
		 * trailing garbage, fail silently */
//		error_msg("Couldnt read header");
		return(NULL);
	}
	archive_offset += 512;

	/* Check header has valid magic, unfortunately some tar files
	 * have empty (0'ed) tar entries at the end, which will
	 * cause this to fail, so fail silently for now
	 */
	if (strncmp(tar.formated.magic, "ustar", 5) != 0) {
#ifdef CONFIG_FEATURE_TAR_OLDGNU_COMPATABILITY
		if (strncmp(tar.formated.magic, "\0\0\0\0\0", 5) != 0)
#endif
		return(NULL);
	}

	/* Do checksum on headers */
	for (i =  0; i < 148 ; i++) {
		sum += tar.raw[i];
	}
	sum += ' ' * 8;
	for (i =  156; i < 512 ; i++) {
		sum += tar.raw[i];
	}
	if (sum != strtol(tar.formated.chksum, NULL, 8)) {
		if ( strtol(tar.formated.chksum,NULL,8) != 0 )
			error_msg("Invalid tar header checksum");
                return(NULL);
        }

	/* convert to type'ed variables */
	tar_entry = xcalloc(1, sizeof(file_header_t));



	// tar_entry->name = xstrdup(tar.formated.name);

/*
	parse_mode(tar.formated.mode, &tar_entry->mode);
*/
        tar_entry->mode = 07777 & strtol(tar.formated.mode, NULL, 8);

	tar_entry->uid   = strtol(tar.formated.uid, NULL, 8);
	tar_entry->gid   = strtol(tar.formated.gid, NULL, 8);
	tar_entry->size  = strtol(tar.formated.size, NULL, 8);
	tar_entry->mtime = strtol(tar.formated.mtime, NULL, 8);

	tar_entry->device = (strtol(tar.formated.devmajor, NULL, 8) << 8) +
		strtol(tar.formated.devminor, NULL, 8);

	/* Fix mode, used by the old format */
	switch (tar.formated.typeflag) {
	/* hard links are detected as regular files with 0 size and a link name */
	case '1':
		tar_entry->mode |= S_IFREG ;
		break;
        case 0:
        case '0':

# ifdef CONFIG_FEATURE_TAR_OLDGNU_COMPATABILITY
		if (last_char_is(tar_entry->name, '/')) {
			tar_entry->mode |= S_IFDIR;
		} else
# endif
			tar_entry->mode |= S_IFREG;
		break;
	case '2':
		tar_entry->mode |= S_IFLNK;
		break;
	case '3':
		tar_entry->mode |= S_IFCHR;
		break;
	case '4':
		tar_entry->mode |= S_IFBLK;
		break;
	case '5':
		tar_entry->mode |= S_IFDIR;
		break;
	case '6':
		tar_entry->mode |= S_IFIFO;
		break;
# ifdef CONFIG_FEATURE_TAR_GNU_EXTENSIONS
	case 'L': {
			longname = xmalloc(tar_entry->size + 1);
                        if(fread(longname, tar_entry->size, 1, tar_stream) != 1)
                                return NULL;
			longname[tar_entry->size] = '\0';
			archive_offset += tar_entry->size;

			return(get_header_tar(tar_stream));
		}
	case 'K': {
			linkname = xmalloc(tar_entry->size + 1);
                        if(fread(linkname, tar_entry->size, 1, tar_stream) != 1)
                                return NULL;
			linkname[tar_entry->size] = '\0';
			archive_offset += tar_entry->size;

			return(get_header_tar(tar_stream));
		}
	case 'D':
	case 'M':
	case 'N':
	case 'S':
	case 'V':
		perror_msg("Ignoring GNU extension type %c", tar.formated.typeflag);
# endif
        default:
                perror_msg("Unknown typeflag: 0x%x", tar.formated.typeflag);
                break;

	}


#ifdef CONFIG_FEATURE_TAR_GNU_EXTENSIONS
        if (longname) {
                tar_entry->name = longname;
                longname = NULL;
        } else
#endif
        {
                tar_entry->name = xstrndup(tar.formated.name, 100);

                if (tar.formated.prefix[0]) {
                        char *temp = tar_entry->name;
                        char *prefixTemp = xstrndup(tar.formated.prefix, 155);
                        tar_entry->name = concat_path_file(prefixTemp, temp);
                        free(temp);
                        free(prefixTemp);
                }
        }

	if (linkname) {
		tar_entry->link_name = linkname;
		linkname = NULL;
	} else
	{
		tar_entry->link_name = *tar.formated.linkname != '\0' ?
			xstrndup(tar.formated.linkname, 100) : NULL;
	}

	return(tar_entry);
}

static void
free_header_tar(file_header_t *tar_entry)
{
	if (tar_entry == NULL)
		return;

	free(tar_entry->name);
	if (tar_entry->link_name)
		free(tar_entry->link_name);

	free(tar_entry);
}

char *
deb_extract(const char *package_filename, FILE *out_stream,
	const int extract_function, const char *prefix,
	const char *filename, int *err)
{
	FILE *deb_stream = NULL;
	file_header_t *ar_header = NULL;
	const char **file_list = NULL;
	char *output_buffer = NULL;
	char *ared_file = NULL;
	char ar_magic[8];
	int gz_err;

	*err = 0;

	if (filename != NULL) {
		file_list = xmalloc(sizeof(char *) * 2);
		file_list[0] = filename;
		file_list[1] = NULL;
	}

	if (extract_function & extract_control_tar_gz) {
		ared_file = "control.tar.gz";
	}
	else if (extract_function & extract_data_tar_gz) {
		ared_file = "data.tar.gz";
	} else {
                opkg_msg(ERROR, "Internal error: extract_function=%x\n",
				extract_function);
		*err = -1;
		goto cleanup;
        }

	/* open the debian package to be worked on */
	deb_stream = wfopen(package_filename, "r");
	if (deb_stream == NULL) {
		*err = -1;
		goto cleanup;
	}
	/* set the buffer size */
	setvbuf(deb_stream, NULL, _IOFBF, 0x8000);

	/* check ar magic */
	fread(ar_magic, 1, 8, deb_stream);

	if (strncmp(ar_magic,"!<arch>",7) == 0) {
		archive_offset = 8;

		while ((ar_header = get_header_ar(deb_stream)) != NULL) {
			if (strcmp(ared_file, ar_header->name) == 0) {
				int gunzip_pid = 0;
				FILE *uncompressed_stream;
				/* open a stream of decompressed data */
				uncompressed_stream = gz_open(deb_stream, &gunzip_pid);
				if (uncompressed_stream == NULL) {
					*err = -1;
					goto cleanup;
				}

				archive_offset = 0;
				output_buffer = unarchive(uncompressed_stream,
						out_stream, get_header_tar,
						free_header_tar,
						extract_function, prefix,
						file_list, err);
				fclose(uncompressed_stream);
				gz_err = gz_close(gunzip_pid);
				if (gz_err)
					*err = -1;
				free_header_ar(ar_header);
				break;
			}
			if (fseek(deb_stream, ar_header->size, SEEK_CUR) == -1) {
				opkg_perror(ERROR, "Couldn't fseek into %s", package_filename);
				*err = -1;
				free_header_ar(ar_header);
				goto cleanup;
			}
			free_header_ar(ar_header);
		}
		goto cleanup;
	} else if (strncmp(ar_magic, "\037\213", 2) == 0) {
		/* it's a gz file, let's assume it's an opkg */
		int unzipped_opkg_pid;
		FILE *unzipped_opkg_stream;
		file_header_t *tar_header;
		archive_offset = 0;
		if (fseek(deb_stream, 0, SEEK_SET) == -1) {
			opkg_perror(ERROR, "Couldn't fseek into %s", package_filename);
			*err = -1;
			goto cleanup;
		}
		unzipped_opkg_stream = gz_open(deb_stream, &unzipped_opkg_pid);
		if (unzipped_opkg_stream == NULL) {
			*err = -1;
			goto cleanup;
		}

		/* walk through outer tar file to find ared_file */
		while ((tar_header = get_header_tar(unzipped_opkg_stream)) != NULL) {
                        int name_offset = 0;
                        if (strncmp(tar_header->name, "./", 2) == 0)
                                name_offset = 2;
			if (strcmp(ared_file, tar_header->name+name_offset) == 0) {
				int gunzip_pid = 0;
				FILE *uncompressed_stream;
				/* open a stream of decompressed data */
				uncompressed_stream = gz_open(unzipped_opkg_stream, &gunzip_pid);
				if (uncompressed_stream == NULL) {
					*err = -1;
					goto cleanup;
				}
				archive_offset = 0;

				output_buffer = unarchive(uncompressed_stream,
							  out_stream,
							  get_header_tar,
							  free_header_tar,
							  extract_function,
							  prefix,
							  file_list,
							  err);

				free_header_tar(tar_header);
				fclose(uncompressed_stream);
				gz_err = gz_close(gunzip_pid);
				if (gz_err)
					*err = -1;
				break;
			}
			seek_sub_file(unzipped_opkg_stream, tar_header->size);
			free_header_tar(tar_header);
		}
		fclose(unzipped_opkg_stream);
		gz_err = gz_close(unzipped_opkg_pid);
		if (gz_err)
			*err = -1;

		goto cleanup;
	} else {
		*err = -1;
		error_msg("%s: invalid magic", package_filename);
	}

cleanup:
	if (deb_stream)
		fclose(deb_stream);
	if (file_list)
		free(file_list);

	return output_buffer;
}
