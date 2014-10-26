#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Architecture="all", Depends="b")
o.add(Package="b", Version="1.0", Architecture="all", Depends="c")
o.write_opk()
o.write_list()

opkgcl.update()

opkgcl.install("a")
if opkgcl.is_installed("a"):
	print(__file__, ": Package 'a' installed, despite dependency "
			"upon a package with an unresolved dependency.")
	exit(False)

if opkgcl.is_installed("b"):
	print(__file__, ": Package 'b' installed, "
			"despite unresolved dependency.")
	exit(False)
