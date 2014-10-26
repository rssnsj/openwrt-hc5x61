/* opkg_defines.h - the opkg package management system

   Copyright (C) 2008 OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef OPKG_DEFINES_H
#define OPKG_DEFINES_H

#define OPKG_PKG_EXTENSION ".opk"
#define IPKG_PKG_EXTENSION ".ipk"
#define DPKG_PKG_EXTENSION ".deb"

#define OPKG_LEGAL_PKG_NAME_CHARS "abcdefghijklmnopqrstuvwxyz0123456789.+-"
#define OPKG_PKG_VERSION_SEP_CHAR '_'

#define OPKG_STATE_DIR_PREFIX OPKGLIBDIR"/opkg"
#define OPKG_LISTS_DIR_SUFFIX "lists"
#define OPKG_INFO_DIR_SUFFIX "info"
#define OPKG_STATUS_FILE_SUFFIX "status"

#define OPKG_BACKUP_SUFFIX "-opkg.backup"

#define OPKG_LIST_DESCRIPTION_LENGTH 128

#endif /* OPKG_DEFINES_H */
