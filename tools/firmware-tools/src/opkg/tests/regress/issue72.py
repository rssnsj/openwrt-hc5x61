#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

long_dir = 110*"a"
long_b = 110*"b"
long_filename = long_dir + "/"+ long_b
long_filename2 = long_dir + "/" + 110*"c"

os.mkdir(long_dir)
open(long_filename, "w").close()
os.symlink(long_b, long_filename2)
a = opk.Opk(Package="a", Version="1.0", Architecture="all")
a.write(data_files=[long_dir, long_filename, long_filename2])
os.unlink(long_filename)
os.unlink(long_filename2)
os.rmdir(long_dir)
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
	print(__file__, ": Package 'a' not installed.")
	exit(False)

if not os.path.exists("{}/{}".format(cfg.offline_root, long_dir)):
	print(__file__, ": dir with name longer than 100 "
					"characters not created.")
	exit(False)

if not os.path.exists("{}/{}".format(cfg.offline_root, long_filename)):
	print(__file__, ": file with a name longer than 100 characters, "
				"in dir with name longer than 100 characters, "
				"not created.")
	exit(False)

if not os.path.lexists("{}/{}".format(cfg.offline_root, long_filename2)):
	print(__file__, ": symlink with a name longer than 100 characters, "
				"pointing at a file with a name longer than "
				"100 characters,"
				"in dir with name longer than 100 characters, "
				"not created.")
	exit(False)

linky = os.path.realpath("{}/{}".format(cfg.offline_root, long_filename2))
linky_dst = "{}/{}".format(cfg.offline_root, long_filename)
if linky != linky_dst:
	print(__file__, ": symlink path truncated.")
	exit(False)

opkgcl.remove("a")
