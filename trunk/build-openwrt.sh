#!/bin/bash -e

cd `dirname $0`

hiwifi_root=`pwd`

# By default keep WAN port separated from LAN ports
WAN_AS_LAN_PORT=n
root_squashfs=openwrt-ar71xx-generic-root.squashfs
official_fw=recovery.bin
target_fw=openwrt-tw150v1-recovery.bin

build_openwrt()
{
	# 1. Install required components:
	local missed_pkgs=""
	for deb in build-essential flex gettext libncurses5-dev unzip gawk liblzma-dev; do
		dpkg -s $deb &> /dev/null || missed_pkgs="${missed_pkgs}${deb} "
	done

	if ! [ -z "$missed_pkgs" ]; then
		echo "Some required packages are missing. Please run the following commands to install them:"
		echo "  sudo apt-get update"
		echo "  sudo apt-get install -y $missed_pkgs"
		exit 1
	fi

	# 2. Checkout source code (this is the latest stable version recommended by OpenWrt Wiki):
	[ -e openwrt-ar9331 ] || svn co svn://svn.openwrt.org/openwrt/trunk openwrt-ar9331 -r36145

	# Set the HIWIFI_WAN_AS_LAN_PORT macro in mach-ap83.c
	if [ $WAN_AS_LAN_PORT = y ]; then
		# Uncomment this line if the option is disabled
		( set -x; sed -i 's/^\/\/\(#define[ \t]\+HIWIFI_WAN_AS_LAN_PORT.*\)$/\1/' mach-tw150v1/mach-ap83.c )
	else
		# Comment the line to disable
		( set -x; sed -i 's/^\(#define[ \t]\+HIWIFI_WAN_AS_LAN_PORT.*\)$/\/\/\1/' mach-tw150v1/mach-ap83.c )
	fi

	# Operations under the OpenWrt source directory
	(
		cd openwrt-ar9331

		# 3. Add LuCI (web GUI) for compiling:
		./scripts/feeds update
		./scripts/feeds install -a -p luci

		# 4. Add 'libevent2' (not necessary):
		./scripts/feeds install libevent2

		# 5. Add 'tw150v1' platform support: <<<< IMPORTANT
		local mach_to_replace=target/linux/ar71xx/files/arch/mips/ath79/mach-ap83.c
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
	(
		set -x
		./firmware-builder/tw150v1-buildfw.sh "$hiwifi_root/openwrt-ar9331" "$target_fw" &&
			mv -vf "./firmware-builder/$target_fw" ./
	)
}

# Options
while [ $# -gt 0 ]; do
	case "$1" in
		--wanaslan) WAN_AS_LAN_PORT=y;;
		--help|-h)
			echo "Usage:"
			echo "  $0 [--wanaslan]"
			echo "Options:"
			echo "  --wanaslan       use WAN port as a LAN port for WAN VLAN support"
			exit 0
			;;
		--*|-*)
			echo "*** Undefined option '$1'. Use --help for help."
			exit 1;;
	esac
	shift 1
done

build_openwrt
