#!/bin/bash -e

cd `dirname $0`

hiwifi_root=`pwd`

root_squashfs=openwrt-ar71xx-generic-root.squashfs
official_fw=recovery.bin
target_fw=dreambox-tw150v1-recovery.bin

# 1. Install required components:
for deb in build-essential flex gettext libncurses5-dev unzip gawk liblzma-dev; do
	dpkg -s $deb &> /dev/null || apt-get install -y $deb
done 

# 2. Checkout source code:
[ -e dreambox-ar9331 ] || svn co svn://svn.openwrt.org.cn/dreambox/trunk dreambox-ar9331 

# Operations under the OpenWrt source directory
(
	cd dreambox-ar9331

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

	# 6. Always replace .config with the repository's
	if [ -e .config ]; then
		mv .config .config.bak
		echo "WARNING: .config is updated, backed up as '.config.bak'"
	fi
	cp -vf ../config-dreambox-ar9xxx-ap83 .config

	# Wait for user to interrupt to check the configuration
	echo "--- Waiting for 3s to interrupt if you want to check the configuration"
	sleep 3
	echo "--- No interrupt. Starting..."

	# 7. Build images for AP83 platform that we selected:
	make V=99
)

# 8. Generate firmware image for HiWiFi board:
(
	export openwrt_root="$hiwifi_root/dreambox-ar9331"
	cd firmware-builder
	./tw150v1-buildfw.sh
	mv -vf openwrt-tw150v1-recovery.bin ../dreambox-tw150v1-recovery.bin
) || exit $?

