#!/bin/bash -e

#####################################################
# HiWiFi flash partition/image layout:
#
# 0x0000000-0x0030000 (0x030000) : "u-boot"
# 0x0030000-0x0040000 (0x010000) : "hw_panic"
# 0x0040000-0x0050000 (0x010000) : "Factory"
# 0x0050000-0x0170000 (0x120000) : "kernel"
# 0x0170000-0x0fd0000 (XXXXXXXX) : "rootfs"
# 0x0b60000-0x0fd0000 (XXXXXXXX) : "rootfs_data"
# 0x0fd0000-0x0fe0000 (0x010000) : "hwf_config"
# 0x0fe0000-0x0ff0000 (0x010000) : "bdinfo"
# 0x0ff0000-0x1000000 (0x010000) : "backup"
# 0x0050000-0x0fd0000 (0xf80000) : "firmware"
#
#####################################################

official_fw=recovery.bin
rooted_fw=recovery.bin.ssh

KERNEL_START_BYTES=327680
SQUASHFS_START_BYTES=1507328

create_rooted_firmware()
{
	if [ -n "$1" ]; then
		official_fw="$1"
		rooted_fw="$1.ssh"
	fi

	[ -e "$official_fw" ] || { echo "*** File '$official_fw' not found."; exit 1; }
	
	# Build the firmware tools
	make -C padjffs2
	make -C squashfs-tools || { echo -e "\nTry \"apt-get install liblzma-dev\" and rerun me."; exit 1; }

	# Get prefix partitions: u-boot, hw_panic, factory
	head "$official_fw" -c $KERNEL_START_BYTES > prefix-u_p_f.bin

	# Get prefix partitions: u-boot, hw_panic, factory, kernel
	head "$official_fw" -c $SQUASHFS_START_BYTES > prefix-u_p_f_k.bin

	# Skip 0x160000 bytes to get the SquashFS image
	tail "$official_fw" -c +`expr $SQUASHFS_START_BYTES + 1` > squashfs.orig

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
	cat prefix-u_p_f_k.bin squashfs.128k > "$rooted_fw"

	[ -d /tftpboot ] && cp -vf "$rooted_fw" /tftpboot/recovery.bin
	ln -sf "$rooted_fw" recovery.bin

	#rm -vf prefix-u_p_f_k.bin squashfs.orig squashfs.raw squashfs.128k
}

clean_env()
{
	rm -vf prefix-u_p_f.bin prefix-u_p_f_k.bin squashfs.orig squashfs.raw squashfs.128k "$rooted_fw"
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

