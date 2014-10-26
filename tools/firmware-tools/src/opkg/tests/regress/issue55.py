#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

long_filename = 110*"a"

os.symlink(long_filename, "linky")
a = opk.Opk(Package="a", Version="1.0", Architecture="all")
a.write(data_files=["linky"])
os.unlink("linky")
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
	print(__file__, ": Package 'a' not installed.")
	exit(False)

if not os.path.lexists("{}/linky".format(cfg.offline_root)):
	print(__file__, ": symlink to file with a name longer than 100 "
					"characters not created.")
	exit(False)

opkgcl.remove("a")
