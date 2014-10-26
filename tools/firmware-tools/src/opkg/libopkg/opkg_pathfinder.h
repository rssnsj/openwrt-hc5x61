/* opkg_pathfinder.h - the opkg package management system


   Copyright (C) 2009 Camille Moncelier <moncelier@devlife.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef OPKG_PATHFINDER_H
#define OPKG_PATHFINDER_H

#include "config.h"

#if defined(HAVE_OPENSSL)
int pkcs7_pathfinder_verify_signers(PKCS7* p7);
#endif

#if defined(HAVE_SSLCURL)
CURLcode curl_ssl_ctx_function(CURL * curl, void * sslctx, void * parm);
#endif


#endif
