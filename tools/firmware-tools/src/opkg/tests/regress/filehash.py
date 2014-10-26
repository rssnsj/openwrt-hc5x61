#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

open("asdf", "w").close()
a = opk.Opk(Package="a", Version="1.0", Architecture="all")
a.write(data_files=["asdf"])
b = opk.Opk(Package="b", Version="1.0", Architecture="all")
b.write(data_files=["asdf"])
os.unlink("asdf")
opkgcl.install("a_1.0_all.opk")

if not opkgcl.is_installed("a"):
	print(__file__, ": Package 'a' not installed.")
	exit(False)

if not os.path.exists("{}/asdf".format(cfg.offline_root)):
	print(__file__, ": asdf not created.")
	exit(False)

opkgcl.install("b_1.0_all.opk", "--force-overwrite")

if "{}/asdf".format(cfg.offline_root) not in opkgcl.files("b"):
	print(__file__, ": asdf not claimed by ``b''.")
	exit(False)

if "{}/asdf".format(cfg.offline_root) in opkgcl.files("a"):
	print(__file__, ": asdf is still claimed by ``a''.")
	exit(False)

opkgcl.remove("b")
opkgcl.remove("a")
