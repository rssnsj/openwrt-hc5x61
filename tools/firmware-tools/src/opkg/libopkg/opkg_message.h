/* opkg_message.h - the opkg package management system

   Copyright (C) 2009 Ubiq Technologies <graham.gower@gmail.com>
   Copyright (C) 2003 Daniele Nicolodi <daniele@grinta.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef _OPKG_MESSAGE_H_
#define _OPKG_MESSAGE_H_

#include <string.h>
#include <errno.h>

typedef enum {
	ERROR,	/* error conditions */
	NOTICE,	/* normal but significant condition */
	INFO,	/* informational message */
	DEBUG,	/* debug level message */
	DEBUG2,	/* more debug level message */
} message_level_t;

void free_error_list(void);
void print_error_list(void);
void opkg_message(message_level_t level, const char *fmt, ...)
				__attribute__ ((format (printf, 2, 3)));

#define opkg_msg(l, fmt, args...) \
	do { \
		if (l == NOTICE) \
			opkg_message(l, fmt, ##args); \
		else \
			opkg_message(l, "%s: "fmt, __FUNCTION__, ##args); \
	} while (0)

#define opkg_perror(l, fmt, args...) \
	opkg_msg(l, fmt": %s.\n", ##args, strerror(errno))

#endif /* _OPKG_MESSAGE_H_ */
