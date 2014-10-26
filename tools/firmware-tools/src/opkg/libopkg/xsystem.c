/* xsystem.c - system(3) with error messages

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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "xsystem.h"
#include "libbb/libbb.h"

/* Like system(3), but with error messages printed if the fork fails
   or if the child process dies due to an uncaught signal. Also, the
   return value is a bit simpler:

   -1 if there was any problem
   Otherwise, the 8-bit return value of the program ala WEXITSTATUS
   as defined in <sys/wait.h>.
*/
int
xsystem(const char *argv[])
{
	int status;
	pid_t pid;

	pid = vfork();

	switch (pid) {
	case -1:
		opkg_perror(ERROR, "%s: vfork", argv[0]);
		return -1;
	case 0:
		/* child */
		execvp(argv[0], (char*const*)argv);
		_exit(-1);
	default:
		/* parent */
		break;
	}

	if (waitpid(pid, &status, 0) == -1) {
		opkg_perror(ERROR, "%s: waitpid", argv[0]);
		return -1;
	}

	if (WIFSIGNALED(status)) {
		opkg_msg(ERROR, "%s: Child killed by signal %d.\n",
			argv[0], WTERMSIG(status));
		return -1;
	}

	if (!WIFEXITED(status)) {
		/* shouldn't happen */
		opkg_msg(ERROR, "%s: Your system is broken: got status %d "
			"from waitpid.\n", argv[0], status);
		return -1;
	}

	return WEXITSTATUS(status);
}
