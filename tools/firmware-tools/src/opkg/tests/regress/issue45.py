#!/usr/bin/python3

import os
import opk, cfg, opkgcl

opk.regress_init()

o = opk.OpkGroup()
o.add(Package="a", Version="1.0", Architecture="all", Depends="b")
o.add(Package="b", Version="1.0", Architecture="all")
o.write_opk()
o.write_list()

opkgcl.update()

(status, output) = opkgcl.opkgcl("install a")
ln_a = output.find("Configuring a")
ln_b = output.find("Configuring b")

if ln_a == -1:
	print(__file__, ": Didn't see package 'a' get configured.")
	exit(False)

if ln_b == -1:
	print(__file__, ": Didn't see package 'b' get configured.")
	exit(False)

if ln_a < ln_b:
	print(__file__, ": Packages 'a' and 'b' configured in wrong order.")
	exit(False)

opkgcl.remove("a")
opkgcl.remove("b")
