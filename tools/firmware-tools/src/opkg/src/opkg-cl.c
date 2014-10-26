/* opkg-cl.c - the opkg package management system

   Florian Boor
   Copyright (C) 2003 kernel concepts

   Carl D. Worth
   Copyright 2001 University of Southern California

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   opkg command line frontend using libopkg
*/

#include "config.h"

#include <stdio.h>
#include <getopt.h>

#include "opkg_conf.h"
#include "opkg_cmd.h"
#include "file_util.h"
#include "opkg_message.h"
#include "opkg_download.h"
#include "../libbb/libbb.h"

enum {
	ARGS_OPT_FORCE_MAINTAINER = 129,
	ARGS_OPT_FORCE_DEPENDS,
	ARGS_OPT_FORCE_OVERWRITE,
	ARGS_OPT_FORCE_DOWNGRADE,
	ARGS_OPT_FORCE_REINSTALL,
	ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES,
	ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES,
	ARGS_OPT_FORCE_SPACE,
	ARGS_OPT_FORCE_POSTINSTALL,
	ARGS_OPT_FORCE_REMOVE,
	ARGS_OPT_ADD_ARCH,
	ARGS_OPT_ADD_DEST,
	ARGS_OPT_NOACTION,
	ARGS_OPT_DOWNLOAD_ONLY,
	ARGS_OPT_NODEPS,
	ARGS_OPT_AUTOREMOVE,
	ARGS_OPT_CACHE,
};

static struct option long_options[] = {
	{"query-all", 0, 0, 'A'},
	{"autoremove", 0, 0, ARGS_OPT_AUTOREMOVE},
	{"cache", 1, 0, ARGS_OPT_CACHE},
	{"conf-file", 1, 0, 'f'},
	{"conf", 1, 0, 'f'},
	{"dest", 1, 0, 'd'},
        {"force-maintainer", 0, 0, ARGS_OPT_FORCE_MAINTAINER},
        {"force_maintainer", 0, 0, ARGS_OPT_FORCE_MAINTAINER},
	{"force-depends", 0, 0, ARGS_OPT_FORCE_DEPENDS},
	{"force_depends", 0, 0, ARGS_OPT_FORCE_DEPENDS},
	{"force-overwrite", 0, 0, ARGS_OPT_FORCE_OVERWRITE},
	{"force_overwrite", 0, 0, ARGS_OPT_FORCE_OVERWRITE},
	{"force_downgrade", 0, 0, ARGS_OPT_FORCE_DOWNGRADE},
	{"force-downgrade", 0, 0, ARGS_OPT_FORCE_DOWNGRADE},
	{"force-reinstall", 0, 0, ARGS_OPT_FORCE_REINSTALL},
	{"force_reinstall", 0, 0, ARGS_OPT_FORCE_REINSTALL},
	{"force-space", 0, 0, ARGS_OPT_FORCE_SPACE},
	{"force_space", 0, 0, ARGS_OPT_FORCE_SPACE},
	{"recursive", 0, 0, ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES},
	{"force-removal-of-dependent-packages", 0, 0,
		ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES},
	{"force_removal_of_dependent_packages", 0, 0,
		ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES},
	{"force-removal-of-essential-packages", 0, 0,
		ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES},
	{"force_removal_of_essential_packages", 0, 0,
		ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES},
	{"force-postinstall", 0, 0, ARGS_OPT_FORCE_POSTINSTALL},
	{"force_postinstall", 0, 0, ARGS_OPT_FORCE_POSTINSTALL},
	{"force-remove", 0, 0, ARGS_OPT_FORCE_REMOVE},
	{"force_remove", 0, 0, ARGS_OPT_FORCE_REMOVE},
	{"noaction", 0, 0, ARGS_OPT_NOACTION},
	{"download-only", 0, 0, ARGS_OPT_DOWNLOAD_ONLY},
	{"nodeps", 0, 0, ARGS_OPT_NODEPS},
	{"offline", 1, 0, 'o'},
	{"offline-root", 1, 0, 'o'},
	{"add-arch", 1, 0, ARGS_OPT_ADD_ARCH},
	{"add-dest", 1, 0, ARGS_OPT_ADD_DEST},
	{"test", 0, 0, ARGS_OPT_NOACTION},
	{"tmp-dir", 1, 0, 't'},
	{"tmp_dir", 1, 0, 't'},
	{"verbosity", 2, 0, 'V'},
	{"version", 0, 0, 'v'},
	{0, 0, 0, 0}
};

static int
args_parse(int argc, char *argv[])
{
	int c;
	int option_index = 0;
	int parse_err = 0;
	char *tuple, *targ;

	while (1) {
		c = getopt_long_only(argc, argv, "Ad:f:no:p:t:vV::",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'A':
			conf->query_all = 1;
			break;
		case 'd':
			conf->dest_str = xstrdup(optarg);
			break;
		case 'f':
			conf->conf_file = xstrdup(optarg);
			break;
		case 'o':
			conf->offline_root = xstrdup(optarg);
			break;
		case 't':
			conf->tmp_dir = xstrdup(optarg);
			break;
		case 'v':
			printf("opkg version %s\n", VERSION);
			exit(0);
		case 'V':
			conf->verbosity = INFO;
			if (optarg != NULL)
				conf->verbosity = atoi(optarg);
			break;
		case ARGS_OPT_AUTOREMOVE:
			conf->autoremove = 1;
			break;
		case ARGS_OPT_CACHE:
			free(conf->cache);
			conf->cache = xstrdup(optarg);
			break;
		case ARGS_OPT_FORCE_MAINTAINER:
			conf->force_maintainer = 1;
			break;
		case ARGS_OPT_FORCE_DEPENDS:
			conf->force_depends = 1;
			break;
		case ARGS_OPT_FORCE_OVERWRITE:
			conf->force_overwrite = 1;
			break;
		case ARGS_OPT_FORCE_DOWNGRADE:
			conf->force_downgrade = 1;
			break;
		case ARGS_OPT_FORCE_REINSTALL:
			conf->force_reinstall = 1;
			break;
		case ARGS_OPT_FORCE_REMOVAL_OF_ESSENTIAL_PACKAGES:
			conf->force_removal_of_essential_packages = 1;
			break;
		case ARGS_OPT_FORCE_REMOVAL_OF_DEPENDENT_PACKAGES:
			conf->force_removal_of_dependent_packages = 1;
			break;
		case ARGS_OPT_FORCE_SPACE:
			conf->force_space = 1;
			break;
		case ARGS_OPT_FORCE_POSTINSTALL:
			conf->force_postinstall = 1;
			break;
		case ARGS_OPT_FORCE_REMOVE:
			conf->force_remove = 1;
			break;
		case ARGS_OPT_NODEPS:
			conf->nodeps = 1;
			break;
		case ARGS_OPT_ADD_ARCH:
		case ARGS_OPT_ADD_DEST:
			tuple = xstrdup(optarg);
			if ((targ = strchr(tuple, ':')) != NULL) {
				*targ++ = 0;
				if ((strlen(tuple) > 0) && (strlen(targ) > 0)) {
					nv_pair_list_append(
						(c == ARGS_OPT_ADD_ARCH)
							? &conf->arch_list : &conf->tmp_dest_list,
						tuple, targ);
				}
			}
			free(tuple);
			break;
		case ARGS_OPT_NOACTION:
			conf->noaction = 1;
			break;
        case ARGS_OPT_DOWNLOAD_ONLY:
			conf->download_only = 1;
			break;
		case ':':
			parse_err = -1;
			break;
		case '?':
			parse_err = -1;
			break;
		default:
			printf("Confusion: getopt_long returned %d\n", c);
		}
	}

	if(!conf->conf_file && !conf->offline_root)
		conf->conf_file = xstrdup("/etc/opkg.conf");

	if (parse_err)
		return parse_err;
	else
		return optind;
}

static void
usage()
{
	printf("usage: opkg [options...] sub-command [arguments...]\n");
	printf("where sub-command is one of:\n");

	printf("\nPackage Manipulation:\n");
	printf("\tupdate			Update list of available packages\n");
	printf("\tupgrade <pkgs>		Upgrade packages\n");
	printf("\tinstall <pkgs>		Install package(s)\n");
	printf("\tconfigure <pkgs>	Configure unpacked package(s)\n");
	printf("\tremove <pkgs|regexp>	Remove package(s)\n");
	printf("\tflag <flag> <pkgs>	Flag package(s)\n");
	printf("\t <flag>=hold|noprune|user|ok|installed|unpacked (one per invocation)\n");

	printf("\nInformational Commands:\n");
	printf("\tlist			List available packages\n");
	printf("\tlist-installed		List installed packages\n");
	printf("\tlist-upgradable		List installed and upgradable packages\n");
	printf("\tlist-changed-conffiles	List user modified configuration files\n");
	printf("\tfiles <pkg>		List files belonging to <pkg>\n");
	printf("\tsearch <file|regexp>	List package providing <file>\n");
	printf("\tinfo [pkg|regexp]	Display all info for <pkg>\n");
	printf("\tstatus [pkg|regexp]	Display all status for <pkg>\n");
	printf("\tdownload <pkg>		Download <pkg> to current directory\n");
	printf("\tcompare-versions <v1> <op> <v2>\n");
	printf("\t                    compare versions using <= < > >= = << >>\n");
	printf("\tprint-architecture	List installable package architectures\n");
	printf("\tdepends [-A] [pkgname|pat]+\n");
	printf("\twhatdepends [-A] [pkgname|pat]+\n");
	printf("\twhatdependsrec [-A] [pkgname|pat]+\n");
	printf("\twhatrecommends[-A] [pkgname|pat]+\n");
	printf("\twhatsuggests[-A] [pkgname|pat]+\n");
	printf("\twhatprovides [-A] [pkgname|pat]+\n");
	printf("\twhatconflicts [-A] [pkgname|pat]+\n");
	printf("\twhatreplaces [-A] [pkgname|pat]+\n");

	printf("\nOptions:\n");
	printf("\t-A			Query all packages not just those installed\n");
	printf("\t-V[<level>]		Set verbosity level to <level>.\n");
	printf("\t--verbosity[=<level>]	Verbosity levels:\n");
	printf("\t				0 errors only\n");
	printf("\t				1 normal messages (default)\n");
	printf("\t				2 informative messages\n");
	printf("\t				3 debug\n");
	printf("\t				4 debug level 2\n");
	printf("\t-f <conf_file>		Use <conf_file> as the opkg configuration file\n");
	printf("\t--conf <conf_file>\n");
	printf("\t--cache <directory>	Use a package cache\n");
	printf("\t-d <dest_name>		Use <dest_name> as the the root directory for\n");
	printf("\t--dest <dest_name>	package installation, removal, upgrading.\n");
	printf("				<dest_name> should be a defined dest name from\n");
	printf("				the configuration file, (but can also be a\n");
	printf("				directory name in a pinch).\n");
	printf("\t-o <dir>		Use <dir> as the root directory for\n");
	printf("\t--offline-root <dir>	offline installation of packages.\n");
	printf("\t--add-arch <arch>:<prio>	Register architecture with given priority\n");
	printf("\t--add-dest <name>:<path>	Register destination with given path\n");

	printf("\nForce Options:\n");
	printf("\t--force-depends		Install/remove despite failed dependencies\n");
	printf("\t--force-maintainer	Overwrite preexisting config files\n");
	printf("\t--force-reinstall	Reinstall package(s)\n");
	printf("\t--force-overwrite	Overwrite files from other package(s)\n");
	printf("\t--force-downgrade	Allow opkg to downgrade packages\n");
	printf("\t--force-space		Disable free space checks\n");
	printf("\t--force-postinstall	Run postinstall scripts even in offline mode\n");
	printf("\t--force-remove	Remove package even if prerm script fails\n");
	printf("\t--noaction		No action -- test only\n");
	printf("\t--download-only	No action -- download only\n");
	printf("\t--nodeps		Do not follow dependencies\n");
	printf("\t--force-removal-of-dependent-packages\n");
	printf("\t			Remove package and all dependencies\n");
	printf("\t--autoremove		Remove packages that were installed\n");
	printf("\t			automatically to satisfy dependencies\n");
	printf("\t-t			Specify tmp-dir.\n");
	printf("\t--tmp-dir		Specify tmp-dir.\n");

	printf("\n");

	printf(" regexp could be something like 'pkgname*' '*file*' or similar\n");
	printf(" e.g. opkg info 'libstd*' or opkg search '*libop*' or opkg remove 'libncur*'\n");

	/* --force-removal-of-essential-packages	Let opkg remove essential packages.
		Using this option is almost guaranteed to break your system, hence this option
		is not even advertised in the usage statement. */

	exit(1);
}

int
main(int argc, char *argv[])
{
	int opts, err = -1;
	char *cmd_name;
	opkg_cmd_t *cmd;
	int nocheckfordirorfile = 0;
        int noreadfeedsfile = 0;

	if (opkg_conf_init())
		goto err0;

	conf->verbosity = NOTICE;

	opts = args_parse(argc, argv);
	if (opts == argc || opts < 0) {
		fprintf(stderr, "opkg must have one sub-command argument\n");
		usage();
	}

	cmd_name = argv[opts++];

	if (!strcmp(cmd_name,"print-architecture") ||
	    !strcmp(cmd_name,"print_architecture") ||
	    !strcmp(cmd_name,"print-installation-architecture") ||
	    !strcmp(cmd_name,"print_installation_architecture") )
		nocheckfordirorfile = 1;

	if (!strcmp(cmd_name,"flag") ||
	    !strcmp(cmd_name,"configure") ||
	    !strcmp(cmd_name,"remove") ||
	    !strcmp(cmd_name,"files") ||
	    !strcmp(cmd_name,"search") ||
	    !strcmp(cmd_name,"compare_versions") ||
	    !strcmp(cmd_name,"compare-versions") ||
	    !strcmp(cmd_name,"list_installed") ||
	    !strcmp(cmd_name,"list-installed") ||
	    !strcmp(cmd_name,"list_changed_conffiles") ||
	    !strcmp(cmd_name,"list-changed-conffiles") ||
	    !strcmp(cmd_name,"status") )
		noreadfeedsfile = 1;

	cmd = opkg_cmd_find(cmd_name);
	if (cmd == NULL) {
		fprintf(stderr, "%s: unknown sub-command %s\n", argv[0],
			 cmd_name);
		usage();
	}

	conf->pfm = cmd->pfm;

	if (opkg_conf_load())
		goto err0;

	if (!nocheckfordirorfile) {
		if (!noreadfeedsfile) {
			if (pkg_hash_load_feeds())
				goto err1;
		}

		if (pkg_hash_load_status_files())
			goto err1;
	}

	if (cmd->requires_args && opts == argc) {
		fprintf(stderr,
			 "%s: the ``%s'' command requires at least one argument\n",
			 argv[0], cmd_name);
		usage();
	}

	err = opkg_cmd_exec(cmd, argc - opts, (const char **) (argv + opts));

#ifdef HAVE_CURL
	opkg_curl_cleanup();
#endif
err1:
	opkg_conf_deinit();

err0:
	print_error_list();
	free_error_list();

	return err;
}
