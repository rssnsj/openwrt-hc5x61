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

	# 6. Always replace .config with the repository's
	if [ -e .config ]; then
		mv .config .config.bak
		echo "WARNING: .config is updated, backed up as '.config.bak'"
	fi
	cp -vf ../config-openwrt-ar9xxx-ap83 .config

	# Wait for user to interrupt to check the configuration
	echo "--- Waiting for 5s to interrupt if you want to check the configuration"
	sleep 5
	echo "--- No interrupt. Starting..."

	# 7. Build images for AP83 platform that we selected:
	make V=99
)

# 8. Generate firmware image for HiWiFi board:
./firmware-builder/tw150v1-buildfw.sh "$hiwifi_root/openwrt-ar9331" "$target_fw" &&
	mv -vf "./firmware-builder/$target_fw" ./

