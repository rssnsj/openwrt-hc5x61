#!/bin/sh

do_upgrade_bootloader() {
	local loop_min=1
	local loop_max=5

	while [[ $loop_min -le $loop_max ]];
	do
		get_image "$1" | dd bs=2k count=32 conv=sync 2>/dev/null | mtd -q -l write - "${BOOT_NAME:-image}"
		get_image "$1" | dd bs=2k count=32 conv=sync 2>/dev/null | mtd -q -l verify - "${BOOT_NAME:-image}" 2>&1 | grep -qs "Success"
		if [[ "$?" -eq 0 ]]; then
			break
		fi
		loop_min=`expr $loop_min + 1`
	done
}

do_conf_backup() {
	mtdpart="$(find_mtd_part hwf_config)"
	[ -z "$mtdpart" ] && return 1
	mtd -q erase hwf_config
	mtd -q write "$1" hwf_config
}

led_set_attr()
{
	[ -f "/sys/class/leds/$1/$2" ] && echo "$3" > "/sys/class/leds/$1/$2"
}
setled_timer()
{
	led_set_attr $1 "brightness" "255"
	led_set_attr $1 "trigger" "timer"
	led_set_attr $1 "delay_on" "$2"
	led_set_attr $1 "delay_off" "$3"
}

platform_do_upgrade_tw150v1() {
	local board=$(ar71xx_board_name)
	local upgrade_boot=0
	sync

	setled_timer "tw150v1:green:system" 500 2500
	sleep 1
	setled_timer "tw150v1:green:internet" 500 2500
	sleep 1
	setled_timer "tw150v1:green:wlan-2p4" 500 2500

	#if [ -f /tmp/img_has_boot ]; then
	#	img_boot_version="$(get_boot_version "$1")"
	#	local_boot_version="$(tw_boot_version)"
	#
	#	if [ "$img_boot_version" != "$local_boot_version" ]; then
	#		upgrade_boot=1
	#	fi
	#fi

	echo "Begin write flash in sysupgrade"

	if [ "$SAVE_CONFIG" -eq 1 -a -z "$USE_REFRESH" ]; then
		do_conf_backup "$CONF_TAR"
		if [ -f /tmp/img_has_boot ]; then
			#if [ $upgrade_boot -eq 1 ]; then
			#	do_upgrade_bootloader "$1"
			#fi
			get_image "$1" | dd bs=2k skip=64 conv=sync 2>/dev/null | mtd -q -j "$CONF_TAR" write - "${PART_NAME:-image}"
		else
			get_image "$1" | mtd -q -j "$CONF_TAR" write - "${PART_NAME:-image}"
		fi
	else
		if [ -f /tmp/img_has_boot ]; then
			#if [ $upgrade_boot -eq 1 ]; then
			#	do_upgrade_bootloader "$1"
			#fi
			get_image "$1" | dd bs=2k skip=64 conv=sync 2>/dev/null | mtd -q write - "${PART_NAME:-image}"
		else
			get_image "$1" | mtd -q write - "${PART_NAME:-image}"
		fi
	fi

	echo "End write flash in sysupgrade"
}
