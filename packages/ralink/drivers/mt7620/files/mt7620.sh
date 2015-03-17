#!/bin/sh
append DRIVERS "mt7620"

. /lib/wifi/ralink_common.sh

prepare_mt7620() {
	prepare_ralink_wifi mt7620
}

scan_mt7620() {
	scan_ralink_wifi mt7620 mt7620
}


disable_mt7620() {
	disable_ralink_wifi mt7620
}

enable_mt7620() {
	enable_ralink_wifi mt7620 mt7620
}

detect_mt7620() {
#	detect_ralink_wifi mt7620 mt7620
	ssid=mt7620-`ifconfig eth0 | grep HWaddr | cut -c 51- | sed 's/://g'`
	cd /sys/module/
	[ -d $module ] || return
	[ -e /etc/config/wireless ] && return
         cat <<EOF
config wifi-device      mt7620
        option type     mt7620
        option vendor   ralink
        option band     2.4G
        option channel  0
    	option auotch   2

config wifi-iface
        option device   mt7620
        option ifname   ra0
        option network  lan
        option mode     ap
        option ssid     $ssid
        option encryption psk2
        option key      12345678

EOF


}


