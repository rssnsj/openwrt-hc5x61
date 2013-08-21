#!/bin/bash -e

[ -z "$openwrt_root" ] && openwrt_root=~/openwrt-ar9331 || :

root_squashfs=openwrt-ar71xx-generic-root.squashfs
official_fw=recovery.bin
target_fw=openwrt-tw150v1-recovery.bin

# 'prefix-1-2.bin' is required, get it from recovery.bin
if [ ! -f prefix-1-2.bin ]; then
	# Get first 0x20000 bytes: u-boot, bdinfo
	( set -x; head "$official_fw" -c131072 > prefix-1-2.bin )
fi

# Check if it is a valid prefix image or truncated
prefix_size=`stat --format=%s prefix-1-2.bin`
if [ $prefix_size -ne 131072 ]; then
	echo "*** prefix-1-2.bin has invalid size. Please remove it and 'svn up'."
	exit 1
fi

(
	set -x

	cp -vf $openwrt_root/build_dir/target-mips*/linux-ar71xx_generic/vmlinux   vmlinux-hiwifi-tw150v1
	$openwrt_root/staging_dir/host/bin/patch-cmdline vmlinux-hiwifi-tw150v1   "board=tw150v1 console=ttyATH0,115200"
	$openwrt_root/staging_dir/host/bin/lzma   e vmlinux-hiwifi-tw150v1 -lc1 -lp2 -pb2 vmlinux-hiwifi-tw150v1.bin.lzma

	cp $openwrt_root/bin/ar71xx/$root_squashfs ./

	mkimage -A mips -O linux -T kernel -a 0x80060000 -C lzma  -e 0x80060000 -n 'tw150v1-Linux-3.8.3' -d vmlinux-hiwifi-tw150v1.bin.lzma uImage

	raw_size=`stat --format=%s uImage`
	pad_size=`expr 1310720 - $raw_size`
	dd if=/dev/zero of=__pad.tmp bs=$pad_size count=1
	cat uImage __pad.tmp > uImage.1280k
	rm -f __pad.tmp

	cat prefix-1-2.bin uImage.1280k $root_squashfs > $target_fw
) || exit $?

[ -d tftpboot ] && cp -vf $target_fw /tftpboot/recovery.bin || :

echo ""
echo "###############################################"
echo "Target firmware: $target_fw"
echo "###############################################"

