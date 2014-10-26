import tarfile, os
import cfg

class Opk:
	valid_control_fields = ["Package", "Version", "Depends", "Provides",\
			"Replaces", "Conflicts", "Suggests", "Recommends",\
			"Section", "Architecture", "Maintainer", "MD5Sum",\
			"Size", "InstalledSize", "Filename", "Source",\
			"Description", "OE", "Homepage", "Priority",\
			"Conffiles"]

	def __init__(self, **control):
		for k in control.keys():
			if k not in self.valid_control_fields:
				raise Exception("Invalid control field: "
						"{}".format(k))
		self.control = control

	def write(self, tar_not_ar=False, data_files=None):
		filename = "{Package}_{Version}_{Architecture}.opk"\
						.format(**self.control)
		if os.path.exists(filename):
			os.unlink(filename)
		if os.path.exists("control"):
			os.unlink("control")
		if os.path.exists("control.tar.gz"):
			os.unlink("control.tar.gz")
		if os.path.exists("data.tar.gz"):
			os.unlink("data.tar.gz")

		f = open("control", "w")
		for k in self.control.keys():
			f.write("{}: {}\n".format(k, self.control[k]))
		f.close()

		tar = tarfile.open("control.tar.gz", "w:gz")
		tar.add("control")
		tar.close()

		tar = tarfile.open("data.tar.gz", "w:gz")
		if data_files:
			for df in data_files:
				tar.add(df)
		tar.close()


		if tar_not_ar:
			tar = tarfile.open(filename, "w:gz")
			tar.add("control.tar.gz")
			tar.add("data.tar.gz")
			tar.close()
		else:
			os.system("ar q {} control.tar.gz data.tar.gz \
					2>/dev/null".format(filename))

		os.unlink("control")
		os.unlink("control.tar.gz")
		os.unlink("data.tar.gz")

class OpkGroup:
	def __init__(self):
		self.opk_list = []

	def add(self, **control):
		self.opk_list.append(Opk(**control))
	
	def write_opk(self, tar_not_ar=False):
		for o in self.opk_list:
			o.write(tar_not_ar)

	def write_list(self, filename="Packages"):
		f = open(filename, "w")
		for opk in self.opk_list:
			for k in opk.control.keys():
				f.write("{}: {}\n".format(k, opk.control[k]))
			f.write("Filename: {Package}_{Version}_{Architecture}"
					".opk\n".format(**opk.control))
			f.write("\n")
		f.close()


def regress_init():
	"""
	Initialisation and sanity checking.
	"""

	if not os.access(cfg.opkgcl, os.X_OK):
		print("Cannot exec {}".format(cfg.opkgcl))
		exit(False)

	os.chdir(cfg.opkdir)

	os.system("rm -fr {}".format(cfg.offline_root))

	os.makedirs("{}/usr/lib/opkg".format(cfg.offline_root))
	os.makedirs("{}/etc/opkg".format(cfg.offline_root))

	f = open("{}/etc/opkg/opkg.conf".format(cfg.offline_root), "w")
	f.write("arch all 1\n")
	f.write("src test file:{}\n".format(cfg.opkdir))
	f.close()
