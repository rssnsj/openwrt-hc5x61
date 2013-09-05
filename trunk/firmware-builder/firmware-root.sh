#!/bin/bash -e

#####################################################
# HiWiFi flash partition/image layout:
#
#  0x00000000-0x00010000 (0x010000) : "u-boot"
#  0x00010000-0x00020000 (0x010000) : "bdinfo"
#  0x00020000-0x00160000 (0x140000) : "kernel"
#  0x00160000-0x00fe0000 (0xE80000) : "rootfs"
#  0x006e0000-0x00fe0000 (0x900000) : "rootfs_data"
#  0x00fe0000-0x00ff0000 (0x010000) : "nvram"
#  0x00ff0000-0x01000000 (0x010000) : "art"
#  0x00020000-0x00fe0000 (0xFC0000) : "firmware" 
#
#####################################################

official_fw=recovery.bin
rooted_fw=recovery.bin.ssh

create_rooted_firmware()
{
	[ -z "$1" ] || official_fw="$1"

	[ -e "$official_fw" ] || { echo "*** File '$official_fw' not found."; exit 1; }
	
	# Build the firmware tools
	make -C padjffs2
	make -C squashfs-tools || { echo -e "\nTry \"apt-get install liblzma-dev\" and rerun me."; exit 1; }

	# Get first 0x20000 bytes: u-boot, bdinfo
	head "$official_fw" -c131072 > prefix-1-2.bin
	
	# Get first 0x160000 bytes: u-boot, bdinfo, kernel partitions
	head "$official_fw" -c1441792 > prefix-1-2-3.bin

	# Skip 0x160000 bytes to get the SquashFS image
	# 1441793 = 0x160001
	tail "$official_fw" -c+1441793 > squashfs.orig

	if ! file squashfs.orig | awk -F: '{print $2}' | grep -i squashfs >/dev/null; then
		echo "*** Not a valid SquashFS at 0x160001."
		exit 1
	fi

	# Extract the file system, to squashfs-root/
	rm -rf squashfs-root
	./squashfs-tools/unsquashfs squashfs.orig

	# Enable serial console and SSH login
	(
		cd squashfs-root
		set -x
		if ! grep ttyATH0 etc/inittab &> /dev/null; then
			echo "ttyATH0::askfirst:/bin/ash --login" >> etc/inittab
		fi
		if [ ! -e etc/rc.d/S50dropbear ]; then
			ln -sv ../init.d/dropbear etc/rc.d/S50dropbear
		fi
	)

	# Rebuild SquashFS image
	./squashfs-tools/mksquashfs squashfs-root squashfs.raw -nopad -noappend -root-owned -comp xz -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2  -b 256k -processors 1
	./padjffs2/padjffs2 squashfs.raw 4 8 64 128 256
	dd if=squashfs.raw of=squashfs.128k bs=128k conv=sync

	# Combine the prefix data and SquashFS to build the final upgrade image
	cat prefix-1-2-3.bin squashfs.128k > "$rooted_fw"

	cp -vf "$rooted_fw" /tftpboot/recovery.bin

	#rm -vf prefix-1-2-3.bin squashfs.orig squashfs.raw squashfs.128k
}

clean_env()
{
	rm -vf prefix-1-2.bin prefix-1-2-3.bin squashfs.orig squashfs.raw squashfs.128k "$rooted_fw"
	rm -rf squashfs-root
	make clean -C padjffs2
	make clean -C squashfs-tools
}

case "$1" in
	-c)
		clean_env
		;;
	-h|--help)
		echo "Usage:"
		echo " $0 [official_recovery.bin]    build a rooted firmware from the official's"
		echo " $0 -c                         clean temporary and target files"
		echo "Note:"
		echo " Default official image file: $official_fw"
		echo " Default target image file: $rooted_fw"
		;;
	*)
		create_rooted_firmware "$@"
		;;
esac

