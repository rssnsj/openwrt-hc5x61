/* opkg_cmd.h - the opkg package management system

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

#ifndef OPKG_CMD_H
#define OPKG_CMD_H

typedef int (*opkg_cmd_fun_t)(int argc, const char **argv);

struct opkg_cmd
{
    const char *name;
    int requires_args;
    opkg_cmd_fun_t fun;
    unsigned int pfm; /* package field mask */
};
typedef struct opkg_cmd opkg_cmd_t;

opkg_cmd_t *opkg_cmd_find(const char *name);
int opkg_cmd_exec(opkg_cmd_t *cmd, int argc, const char **argv);

extern int opkg_state_changed;
#endif
