#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

open("foo", "w").close()
a1 = opk.Opk(Package="a", Version="1.0", Architecture="all")
a1.write(data_files=["foo"])

opkgcl.install("a_1.0_all.opk")

o = opk.OpkGroup()
a2 = opk.Opk(Package="a", Version="2.0", Architecture="all", Depends="b")
a2.write()
b1 = opk.Opk(Package="b", Version="1.0", Architecture="all")
b1.write(data_files=["foo"])
o.opk_list.append(a2)
o.opk_list.append(b1)
o.write_list()

os.unlink("foo")

opkgcl.update()
opkgcl.upgrade()

if not opkgcl.is_installed("a", "2.0"):
	print(__file__, ": Package 'a_2.0' not installed.")
	exit(False)

foo_fullpath = "{}/foo".format(cfg.offline_root)

if not os.path.exists(foo_fullpath):
	print(__file__, ": File 'foo' incorrectly orphaned.")
	exit(False)

if not foo_fullpath in opkgcl.files("b"):
	print(__file__, ": Package 'b' does not own file 'foo'.")
	exit(False)

opkgcl.remove("a")
opkgcl.remove("b")
