#!/bin/sh
append DRIVERS "mt7612e"

. /lib/wifi/ralink_common.sh

prepare_mt7612e() {
	prepare_ralink_wifi mt7612e
}

scan_mt7612e() {
	scan_ralink_wifi mt7612e mt76x2e
}

disable_mt7612e() {
	disable_ralink_wifi mt7612e
}

enable_mt7612e() {
	enable_ralink_wifi mt7612e mt76x2e
}

detect_mt7612e() {
#	detect_ralink_wifi mt7612e mt76x2e
	cd /sys/module/
	[ -d $module ] || return
	uci get wireless.mt7612e >/dev/null 2>&1 && return
	ifconfig rai0 >/dev/null 2>&1 || return
	cat <<EOF
config wifi-device mt7612e
	option type mt7612e
	option vendor ralink
	option band 5G
	option channel 0
	option autoch 2

config wifi-iface
	option device mt7612e
	option ifname rai0
	option network lan
	option mode ap
	option ssid OpenWrt-5G
	option encryption none

EOF

}

