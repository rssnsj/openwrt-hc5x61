#!/bin/bash -e

export SQUASHFS_ROOT=`pwd`/squashfs-root
ENABLE_ROOT_LOGIN=Y
ENABLE_WIRELESS=N
OPKG_REMOVE_LIST=
OPKG_INSTALL_LIST=
ROOTFS_CMDS=

print_green()
{
	if tty -s 2>/dev/null; then
		echo -ne "\033[32m"
		echo -n "$@"
		echo -e "\033[0m"
	else
		echo "$@"
	fi
}

print_red()
{
	if tty -s 2>/dev/null; then
		echo -ne "\033[31m"
		echo -n "$@"
		echo -e "\033[0m"
	else
		echo "$@"
	fi
}

get_magic_long()
{
	dd bs=4 count=1 2>/dev/null 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"'
}

get_magic_word()
{
	dd bs=2 count=1 2>/dev/null 2>/dev/null | hexdump -v -n 4 -e '1/1 "%02x"'
}


print_help()
{
	local arg0=`basename "$0"`
cat <<EOF
Usage:
 $arg0 <major_model> <ROM_file> ...  patch firmware <ROM_file> and repackage
 $arg0 -c                            clean temporary and target files

Options:
 -o <output_file>          filename of newly built firmware
 -r <package>              remove opkg package, can be multiple
 -i <ipk_file>             install package with ipk file path or URL, can be multiple
 -e                        enable root login
 -w                        enable wireless by default
 -x <commands>             execute commands after all other operations
EOF
}

opkg_exec()
{
	# Get system architecture on first execution
	if [ -z "$MAJOR_ARCH" ]; then
		MAJOR_ARCH=`cat "$SQUASHFS_ROOT"/usr/lib/opkg/status | awk -F': *' '/^Architecture:/&&$2!~/^all$/{print $2}' | sort -u | head -n1`
	fi

	IPKG_INSTROOT="$SQUASHFS_ROOT" IPKG_CONF_DIR="$SQUASHFS_ROOT"/etc IPKG_OFFLINE_ROOT="$SQUASHFS_ROOT" \
	opkg --offline-root "$SQUASHFS_ROOT" --conf "$SQUASHFS_ROOT"/etc/opkg.conf \
		--force-depends --force-overwrite --force-maintainer \
		--add-dest root:/ \
		--add-arch all:100 --add-arch $MAJOR_ARCH:200 "$@" || return $?
}

modify_rootfs()
{
	local __rc=0

	(
		cd $SQUASHFS_ROOT
		# Run customized commands
		sh -c "$ROOTFS_CMDS" || :
	)

	# Uninstall old packages
	local ipkg
	for ipkg in $OPKG_REMOVE_LIST; do
		opkg_exec remove "$ipkg"
	done

	# Install extra ipk packages
	for ipkg in ipk.$MAJOR_ARCH/*.ipk ipk.$MODEL_NAME/*.ipk; do
		[ -f "$ipkg" ] || continue
		opkg_exec install "$ipkg" || __rc=104
	done
	if [ -n "$OPKG_INSTALL_LIST" ]; then
		opkg_exec update
		opkg_exec install $OPKG_INSTALL_LIST || __rc=104
	fi

	# Fix auto-start symlinks for /etc/init.d scripts
	print_green "Checking init.d scripts for newly installed services ..."
	local initsc
	for initsc in $SQUASHFS_ROOT/etc/init.d/*; do
		local initname=`basename "$initsc"`
		local start_no=`awk -F= '/^START=/{print $2; exit}' "$initsc"`
		local stop_no=`awk -F= '/^STOP=/{print $2; exit}' "$initsc"`
		if [ -n "$start_no" -o -n "$stop_no" ] && ! ls -d $SQUASHFS_ROOT/etc/rc.d/*$initname >/dev/null 2>&1; then
			echo "Setting auto-start for '$initname' ..."
			[ -n "$start_no" ] && ln -sf ../init.d/$initname $SQUASHFS_ROOT/etc/rc.d/S$start_no$initname || :
			[ -n "$stop_no" ] && ln -sf ../init.d/$initname $SQUASHFS_ROOT/etc/rc.d/K$stop_no$initname || :
		fi
	done

	# Enable wireless on first startup
	if [ "$ENABLE_WIRELESS" = Y ]; then
		sed -i '/option \+disabled \+1/d;/# *REMOVE THIS LINE/d' $SQUASHFS_ROOT/lib/wifi/mac80211.sh
	fi

	rm -rf $SQUASHFS_ROOT/tmp/*

	return $__rc
}

do_firmware_repack()
{
	local old_romfile=
	local new_romfile=
	local __rc=0

	# Parse options and parameters
	local opt
	while [ $# -gt 0 ]; do
		case "$1" in
			-o)
				shift 1
				new_romfile="$1"
				;;
			-r)
				shift 1
				OPKG_REMOVE_LIST="$OPKG_REMOVE_LIST$1 "
				;;
			-i)
				shift 1
				OPKG_INSTALL_LIST="$OPKG_INSTALL_LIST$1 "
				;;
			-e)
				ENABLE_ROOT_LOGIN=Y
				;;
			-w)
				ENABLE_WIRELESS=Y
				;;
			-x)
				shift 1
				ROOTFS_CMDS="$1"
				;;
			-*)
				echo "*** Unknown option '$1'."
				exit 1
				;;
			*)
				if [ -z "$old_romfile" ]; then
					old_romfile="$1"
				else
					echo "*** Useless parameter: $1".
					exit 1
				fi
				;;
		esac
		shift 1
	done


	# Download file if the filename starts with "http*://"
	case "$old_romfile" in
		http*://*)
			local romfile_url="$old_romfile"
			old_romfile=`basename "$old_romfile"`
			if [ -f "$old_romfile" ]; then
				print_red "WARNING: File exists, not downloading original file."
			else
				print_green ">>> Downloading file $romfile_url ..."
				wget -4 "$romfile_url" -O "$old_romfile"
			fi
			;;
	esac

	if [ -z "$old_romfile" -o ! -f "$old_romfile" ]; then
		echo "*** Invalid source firmware file '$old_romfile'"
		exit 1
	fi
	[ -z "$new_romfile" ] && new_romfile="$old_romfile.out" || :

	print_green ">>> Analysing source firmware: $old_romfile ..."

	# Check if firmware is in "SquashFS" type
	local fw_magic=`cat "$old_romfile" | get_magic_word`
	if [ "$fw_magic" != "2705" ]; then
		echo "*** Not a valid sysupgrade firmware file."
		exit 1
	fi

	# Search for SquashFS offset
	local squashfs_offset=`hexof 68737173 "$old_romfile"`
	if [ -n "$squashfs_offset" ]; then
		echo "Found SquashFS at $squashfs_offset."
	else
		echo "*** Cannot find SquashFS magic in firmware."
		exit 1
	fi

	print_green ">>> Extracting kernel, rootfs partitions ..."

	# Partition: kernel
	# dd if="$old_romfile" bs=1 count=$squashfs_offset > uImage.bin
	head "$old_romfile" -c$squashfs_offset > uImage.bin
	# Partition: rootfs
	# dd if="$old_romfile" bs=1 skip=$squashfs_offset > root.squashfs.orig
	tail "$old_romfile" -c+`expr $squashfs_offset + 1` > root.squashfs.orig

	print_green ">>> Extracting SquashFS into directory squashfs-root/ ..."
	# Extract the file system, to squashfs-root/
	rm -rf $SQUASHFS_ROOT
	unsquashfs root.squashfs.orig

	#######################################################
	print_green ">>> Patching the firmware ..."
	# Ignore errors of "opkg install" but exits on other errors
	if modify_rootfs; then
		:
	else
		[ $? -eq 104 ] && __rc=104 || exit 1
	fi
	#######################################################

	# Rebuild SquashFS image
	print_green ">>> Repackaging the modified firmware ..."

	mksquashfs $SQUASHFS_ROOT root.squashfs -nopad -noappend -root-owned -comp xz -Xpreset 9 -Xe -Xlc 0 -Xlp 2 -Xpb 2 -b 256k -p '/dev d 755 0 0' -p '/dev/console c 600 0 0 5 1' -processors 1
	cat uImage.bin root.squashfs > "$new_romfile"
	padjffs2 "$new_romfile" 4 8 16 64 128 256

	print_green ">>> Done. New firmware: $new_romfile"

	[ -d /tftpboot ] && cp -vf "$new_romfile" /tftpboot/recovery.bin
	ln -sf "$new_romfile" recovery.bin

	rm -f root.squashfs* uImage.bin

	exit $__rc
}

clean_env()
{
	rm -f recovery.bin *.out
	rm -f root.squashfs* uImage.bin
	rm -rf $SQUASHFS_ROOT
}

case "$1" in
	-c) clean_env;;
	-h|--help) print_help;;
	*) do_firmware_repack "$@";;
esac

