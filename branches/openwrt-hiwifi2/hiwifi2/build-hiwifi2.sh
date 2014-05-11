#!/bin/bash -e

set -x

KDIR=`ls -d ../openwrt-ramips/build_dir/target-mipsel_*`/linux-ramips_mt7620a
HOSTBIN=../openwrt-ramips/staging_dir/host/bin



cp -vf $KDIR/vmlinux-mt7620a_mt7610e ./vmlinux

$HOSTBIN/patch-cmdline vmlinux "board=HC5761 console=ttyS1,115200"
$HOSTBIN/lzma e vmlinux -lc1 -lp2 -pb2 vmlinux-HC5761.bin.lzma
$HOSTBIN/mkimage -A mips -O linux -T kernel -C lzma -a 0x80000000 -e 0x80000000 \
	-n "HC5761"  -d vmlinux-HC5761.bin.lzma \
	vmlinux-HC5761.uImage
cp -vf $KDIR/root.squashfs root.squashfs

(
	dd if=vmlinux-HC5761.uImage bs=1M conv=sync
	dd if=root.squashfs
) > openwrt-ralink-mt7620-HC5761-squashfs-sysupgrade.bin

if [ `stat -c%s openwrt-ralink-mt7620-HC5761-squashfs-sysupgrade.bin` -gt 16318464 ]; then
	echo "Warning: sysup img is too big"
	rm -f openwrt-ralink-mt7620-HC5761-squashfs-sysupgrade.bin
fi

if [ `stat -c%s openwrt-ralink-mt7620-HC5761-uboot.bin` -gt 327680 ]; then
	echo "Warning: $(1) is too big"
	exit 1
elif [ `stat -c%s openwrt-ralink-mt7620-HC5761-squashfs-sysupgrade.bin` -gt 20000000 ]; then
	echo "Warning: $(3) is too big"
	exit 1
fi

(
	dd if=openwrt-ralink-mt7620-HC5761-uboot.bin bs=327680 conv=sync
	dd if=openwrt-ralink-mt7620-HC5761-squashfs-sysupgrade.bin
) > openwrt-ralink-mt7620-HC5761-squashfs-flashsmt.bin

ln -sf openwrt-ralink-mt7620-HC5761-squashfs-flashsmt.bin recovery.bin

exit

###############################################################

# 1. drivers/mtd/devices/m25p80.c
#    { "w25q128", INFO(0xc84018, 0, 64 * 1024, 256, SECT_4K) },
#
# 2. arch/mips/ralink/rt305x.c
#    FUNC("gpio uartf", RT305X_GPIO_MODE_GPIO_UARTF, 7, 8),
#
# 3. target/ramips/dts/mt7620a.dtsi
#    chosen {
#        bootargs = "console=ttyS0,115200";
#    };
#
# 4. target/ramips/dts/MT7620a.dts
#		partition@50000 {
#			label = "firmware";
#			reg = <0x50000 0xf90000>;
#		};
#
#		partition@150000 {
#			label = "rootfs";
#			reg = <0x150000 0xe90000>;
#		};
#
