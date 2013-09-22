#!/bin/bash -e

cd `dirname $0`

build_openwrt()
{
	# 1. Install required components:
	local missed_pkgs=""
	for deb in build-essential flex gettext libncurses5-dev unzip gawk liblzma-dev u-boot-tools; do
		dpkg -s $deb &> /dev/null || missed_pkgs="${missed_pkgs}${deb} "
	done

	if ! [ -z "$missed_pkgs" ]; then
		echo "Some required packages are missing. Please run the following commands to install them:"
		echo "  sudo apt-get update"
		echo "  sudo apt-get install -y $missed_pkgs"
		exit 1
	fi

	# 2. Checkout source code (this is the latest stable version recommended by OpenWrt Wiki):
	[ -e openwrt-ar9331 ] || svn co svn://svn.openwrt.org/openwrt/trunk openwrt-ar9331 -r38114

	# Operations under the OpenWrt source directory
	(
		cd openwrt-ar9331

		# 3. Add LuCI (web GUI) for compiling:
		./scripts/feeds update
		./scripts/feeds install luci

		# 4. Add 'libevent2' (not necessary):
		./scripts/feeds install libevent2

		# 6. Always replace .config with the repository's
		if [ -e .config ]; then
			mv .config .config.bak
			echo "WARNING: .config is updated, backed up as '.config.bak'"
		fi
		cp -vf ../config-openwrt-ar9xxx-wr720n .config

		# Wait for user to interrupt to check the configuration
		echo "--- Waiting for 5s to interrupt if you want to check the configuration"
		sleep 5
		echo "--- No interrupt. Starting..."

		# 7. Build images for AP83 platform that we selected:
		make V=99
	)
}

# Options
while [ $# -gt 0 ]; do
	case "$1" in
		--help|-h)
			echo "Usage:"
			echo "  $0 "
			exit 0
			;;
		--*|-*)
			echo "*** Undefined option '$1'. Use --help for help."
			exit 1;;
	esac
	shift 1
done

build_openwrt
