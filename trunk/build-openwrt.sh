#!/bin/bash -e

cd `dirname $0`

hiwifi_root=`pwd`

root_squashfs=openwrt-ar71xx-generic-root.squashfs
official_fw=recovery.bin
target_fw=openwrt-tw150v1-recovery.bin

# 1. Install required components:
for deb in build-essential flex gettext libncurses5-dev unzip gawk liblzma-dev; do
	dpkg -s $deb &> /dev/null || apt-get install -y $deb
done 

# 2. Checkout source code:
[ -e openwrt-ar9331 ] || svn co svn://svn.openwrt.org/openwrt/trunk openwrt-ar9331 -r36145

# Operations under the OpenWrt source directory
(
	cd openwrt-ar9331

	# 3. Add LuCI (web GUI) for compiling:
	./scripts/feeds update
	./scripts/feeds install -a -p luci

	# 4. Add 'libevent2' (not necessary):
	./scripts/feeds install libevent2

	# 5. Add 'tw150v1' platform support: <<<< IMPORTANT
	mach_to_replace=target/linux/ar71xx/files/arch/mips/ath79/mach-ap83.c
	if ! [ -L $mach_to_replace -a -e $mach_to_replace ]; then
		rm -vf $mach_to_replace
		ln -sv "$hiwifi_root/mach-tw150v1/mach-ap83.c" $mach_to_replace
	fi

	# 6. Check which config file is newer, and replace the old with the newer
	if [ ../config-ar9xxx-mach-ap83 -nt .config ]; then
		cp -vf .config .config.bak
		cp -vf ../config-ar9xxx-mach-ap83 .config
		echo "WARNING: .config is updated, backed up as '.config.bak'"
	fi

	#elif [ ../config-ar9xxx-mach-ap83 -ot .config ]; then
	#	cp -vf ../config-ar9xxx-mach-ap83 ../config-ar9xxx-mach-ap83.bak
	#	cp -vf .config ../config-ar9xxx-mach-ap83
	#	echo "WARNING: config-ar9xxx-mach-ap83 is updated, backed up as 'config-ar9xxx-mach-ap83.bak'"

	# 7. Build images for AP83 platform that we selected:
	#if ! [ -f build_dir/target-mips*/linux-ar71xx_generic/vmlinux -a \
	#	-f bin/ar71xx/$root_squashfs ]; then
		make V=99
	#fi
)

# 8. Generate firmware image for HiWiFi board:
(
	export openwrt_root="$hiwifi_root/openwrt-ar9331"
	cd firmware-builder
	./tw150v1-buildfw.sh
	mv -vf openwrt-tw150v1-recovery.bin ../
) || exit $?

