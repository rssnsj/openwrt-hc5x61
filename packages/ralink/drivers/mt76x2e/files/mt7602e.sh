#!/bin/sh
append DRIVERS "mt7602e"

. /lib/wifi/ralink_common.sh

prepare_mt7602e() {
	prepare_ralink_wifi mt7602e
}

scan_mt7602e() {
	scan_ralink_wifi mt7602e mt76x2e
}

disable_mt7602e() {
	disable_ralink_wifi mt7602e
}

enable_mt7602e() {
	enable_ralink_wifi mt7602e mt76x2e
}

detect_mt7602e() {
#	detect_ralink_wifi mt7602e mt76x2e
	ssid=mt7602e-`ifconfig eth0 | grep HWaddr | cut -c 51- | sed 's/://g'`
	cd  /sys/module/
	[ -d $module ] || return
        [ -e /etc/config/wireless ] && return
         cat <<EOF
config wifi-device      mt7602e
        option type     mt7602e
        option vendor   ralink
        option band     2.4G
        option channel  0
        option autoch   2

config wifi-iface
        option device   mt7602e
        option ifname   ra0
        option network  lan
        option mode     ap
        option ssid     $ssid
        option encryption psk2
        option key      12345678

EOF

}


