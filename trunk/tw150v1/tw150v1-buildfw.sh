#!/bin/bash -e

if [ $# -lt 2 ]; then
	echo "Usage: tw150v1-buildfw <openwrt_root> <target_fw_name>"
	exit 1
fi

root_squashfs=openwrt-ar71xx-generic-root.squashfs
official_fw=recovery.bin
openwrt_root=
target_fw=

check_exec_or_exit()
{
	# Check the host tools
	local i
	for i in "$@"; do
		[ -x "$i" ] || { echo "*** Cannot find the executable: $i"; return 1; }
	done
	return 0
}

# Inject custormized configs into root filesystem's directory,
#  and repackage it with the filename as given.
# $1: directory of the filesystem tree
# $2: target squashfs filename
inject_config_and_pack_tw150v1()
{
	if ! [ -d "$1" ]; then
		echo "*** Directory '$1' does not exist."
		return 1
	fi
	local fs_root_dir="$1"
	local target_squashfs="$2"

	local MKSQUASHFS="$openwrt_root"/staging_dir/host/bin/mksquashfs4
	local PADJFFS="$openwrt_root"/staging_dir/host/bin/padjffs2

	# Check the host tools
	check_exec_or_exit "$MKSQUASHFS" "$PADJFFS" || return 1

	(
		cd "$1"

		# LED configuration
		( [ -e etc/config/system ] && grep '^config led' etc/config/system >/dev/null ) ||
			cat >> etc/config/system <<EOF

config led
	option sysfs 'tw150v1:green:system'
	option default '1'
	option name 'system'
	option trigger 'timer'
	option delayon '2000'
	option delayoff '1000'

config led
	option default '0'
	option name 'wan'
	option sysfs 'tw150v1:green:internet'
	option trigger 'netdev'
	option dev 'eth1'
	option mode 'link tx rx'

config led
	option default '0'
	option name 'wireless'
	option sysfs 'tw150v1:green:wlan-2p4'
	option trigger 'phy0tpt'

EOF

		# Initialize LAN configuration
		( [ -e etc/config/network ] && grep '^config interface' etc/config/network >/dev/null ) ||
			cat >> etc/config/network <<EOF

config interface 'loopback'
	option ifname 'lo'
	option proto 'static'
	option ipaddr '127.0.0.1'
	option netmask '255.0.0.0'

config interface 'lan'
	option ifname 'eth0'
	option type 'bridge'
	option proto 'static'
	option netmask '255.255.255.0'
	option ipaddr '192.168.1.1'

config interface 'wan'
	option ifname 'eth1'
	option proto 'dhcp'
EOF

		# Switch configuration, otherwise traffic between LAN
		#  ports cannot be forwarded.
		grep '^config switch' etc/config/network >/dev/null ||
			cat >> etc/config/network <<EOF

config switch
	option name 'eth0'
	option reset '1'
	option enable_vlan '1'

config switch_vlan
	option device 'eth0'
	option vlan '1'
	option ports '0 1 2 3 4'

EOF

		# Create the /etc/config/fstab to make menu
		#  "System -> Mount Point" appear.
		[ -e etc/config/fstab ] ||
			cat > etc/config/fstab <<EOF
config global
	option anon_swap '0'
	option anon_mount '0'
	option check_fs '0'

EOF
		#
	)

	"$MKSQUASHFS" "$fs_root_dir" __squashfs -nopad -noappend -root-owned -comp xz -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2  -b 256k -processors 1
	"$PADJFFS" __squashfs 4 8 64 128 256
	dd if=__squashfs of="$target_squashfs" bs=128k conv=sync
	rm -vf __squashfs
}

build_firmware_tw150v1()
{
	# Always switch to the script's directory
	cd `dirname $0`

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

	cp -vf $openwrt_root/build_dir/target-mips*/linux-ar71xx_generic/vmlinux   vmlinux-hiwifi-tw150v1
	$openwrt_root/staging_dir/host/bin/patch-cmdline vmlinux-hiwifi-tw150v1   "board=tw150v1 console=ttyATH0,115200"
	$openwrt_root/staging_dir/host/bin/lzma   e vmlinux-hiwifi-tw150v1 -lc1 -lp2 -pb2 vmlinux-hiwifi-tw150v1.bin.lzma

	# We might prefer to inject some initial configurations rather than just copy it.
	#cp $openwrt_root/bin/ar71xx/$root_squashfs ./
	inject_config_and_pack_tw150v1  $openwrt_root/build_dir/target-mips*/root-ar71xx  ./$root_squashfs

	mkimage -A mips -O linux -T kernel -a 0x80060000 -C lzma  -e 0x80060000 \
		-n 'tw150v1-Linux-3.8.3' -d vmlinux-hiwifi-tw150v1.bin.lzma uImage

	raw_size=`stat --format=%s uImage`
	pad_size=`expr 1310720 - $raw_size`
	dd if=/dev/zero of=__pad.tmp bs=$pad_size count=1
	cat uImage __pad.tmp > uImage.1280k
	rm -vf __pad.tmp

	cat prefix-1-2.bin uImage.1280k $root_squashfs > $target_fw

	[ -d tftpboot ] && cp -vf $target_fw /tftpboot/recovery.bin || :
	echo ""
	echo "###############################################"
	echo "Target firmware: $target_fw"
	echo "###############################################"
}

#
# Retrieve the absolute OpenWrt root path
openwrt_root="$1"
case "$openwrt_root" in
	/*) : ;;
	 *) openwrt_root=`cd "$openwrt_root" && pwd` ;;
esac
if [ ! -d "$openwrt_root" ]; then
	echo "*** '$openwrt_root' is not a valid directory."
	exit 1
fi
# The firmware image name
target_fw="$2"

build_firmware_tw150v1 "$@"
