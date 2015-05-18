# this file will be included in 
#     /lib/wifi/mt{chipname}.sh

repair_wireless_uci() {
    echo "repair_wireless_uci" >>/tmp/wifi.log
    vifs=`uci show wireless | grep "=wifi-iface" | sed -n "s/=wifi-iface//gp"`
    echo $vifs >>/tmp/wifi.log

    ifn5g=0
    ifn2g=0
    for vif in $vifs; do
        local netif nettype device netif_new
        echo  "<<<<<<<<<<<<<<<<<" >>/tmp/wifi.log
        netif=`uci -q get ${vif}.ifname`
        nettype=`uci -q get ${vif}.network`
        device=`uci -q get ${vif}.device`
        if [ "$device" == "" ]; then
            echo "device cannot be empty!!" >>/tmp/wifi.log
            return
        fi
        echo "device name $device!!" >>/tmp/wifi.log
        echo "netif $netif" >>/tmp/wifi.log
        echo "nettype $nettype" >>/tmp/wifi.log
    
        case "$device" in
            mt7620 | mt7602e | mt7603e | mt7628 | mt7688)
                netif_new="ra"${ifn2g}
                ifn2g=$(( $ifn2g + 1 ))
                ;;
            mt7610e | mt7612e )
                netif_new="rai"${ifn5g}
                ifn5g=$(( $ifn5g + 1 ))
                ;;
            * )
                echo "device $device not recognized!! " >>/tmp/wifi.log
                ;;
        esac                    
    
        echo "ifn5g = ${ifn5g}, ifn2g = ${ifn2g}" >>/tmp/wifi.log
        echo "netif_new = ${netif_new}" >>/tmp/wifi.log
            
        if [ "$netif" == "" ]; then
            echo "ifname empty, we'll fix it with ${netif_new}" >>/tmp/wifi.log
            uci -q set ${vif}.ifname=${netif_new}
        fi
        if [ "$nettype" == "" ]; then
            nettype="lan"
            echo "nettype empty, we'll fix it with ${nettype}" >>/tmp/wifi.log
            uci -q set ${vif}.network=${nettype}
        fi
        echo  ">>>>>>>>>>>>>>>>>" >>/tmp/wifi.log
    done
    uci changes >>/tmp/wifi.log
    uci commit
}


sync_uci_with_dat() {
    echo "sync_uci_with_dat($1,$2,$3,$4)" >>/tmp/wifi.log
    local device="$1"
    local datpath="$2"
    uci2dat -d $device -f $datpath > /tmp/uci2dat.log
}



chk8021x() {
        local x8021x="0" encryption device="$1" prefix
        #vifs=`uci show wireless | grep "=wifi-iface" | sed -n "s/=wifi-iface//gp"`
        echo "u8021x dev $device" > /tmp/802.$device.log
        config_get vifs "$device" vifs
        for vif in $vifs; do
                config_get ifname $vif ifname
                echo "ifname = $ifname" >> /tmp/802.$device.log
                config_get encryption $vif encryption
                echo "enc = $encryption" >> /tmp/802.$device.log
                case "$encryption" in
                        wpa+*)
                                [ "$x8021x" == "0" ] && x8021x=1
                                echo 111 >> /tmp/802.$device.log
                                ;;
                        wpa2+*)
                                [ "$x8021x" == "0" ] && x8021x=1
                                echo 1l2 >> /tmp/802.$device.log
                                ;;
                        wpa-mixed*)
                                [ "$x8021x" == "0" ] && x8021x=1
                                echo 1l3 >> /tmp/802.$device.log
                                ;;
                esac
                ifpre=$(echo $ifname | cut -c1-3)
                echo "prefix = $ifpre" >> /tmp/802.$device.log
                if [ "$ifpre" == "rai" ]; then
                    prefix="rai"
                else
                    prefix="ra"
                fi
                if [ "1" == "$x8021x" ]; then
                    break
                fi
        done
        echo "x8021x $x8021x, pre $prefix" >>/tmp/802.$device.log
        if [ "1" == $x8021x ]; then
            if [ "$prefix" == "ra" ]; then
                echo "killall 8021xd" >>/tmp/802.$device.log
                killall 8021xd
                echo "/bin/8021xd -d 9" >>/tmp/802.$device.log
                /bin/8021xd -d 9 >> /tmp/802.$device.log 2>&1
            else # $prefixa == rai
                echo "killall 8021xdi" >>/tmp/802.$device.log
                killall 8021xdi
                echo "/bin/8021xdi -d 9" >>/tmp/802.$device.log
                /bin/8021xdi -d 9 >> /tmp/802.$device.log 2>&1
            fi
        else
            if [ "$prefix" == "ra" ]; then
                echo "killall 8021xd" >>/tmp/802.$device.log
                killall 8021xd
            else # $prefixa == rai
                echo "killall 8021xdi" >>/tmp/802.$device.log
                killall 8021xdi
            fi
        fi
}


# $1=device, $2=module
reinit_wifi() {
    echo "reinit_wifi($1,$2,$3,$4)" >>/tmp/wifi.log
    local device="$1"
    local module="$2"
    config_get vifs "$device" vifs

    # shut down all vifs first
    for vif in $vifs; do
        config_get ifname $vif ifname
        ifconfig $ifname down
    done

    # in some case we have to reload drivers. (mbssid eg)
    #ref=`cat /sys/module/$module/refcnt`
    #if [ $ref != "0" ]; then
    #    # but for single driver, we only need to reload once.
    #    echo "$module ref=$ref, skip reload module" >>/tmp/wifi.log
    #else
    #    echo "rmmod $module" >>/tmp/wifi.log
    #    rmmod $module
    #    echo "insmod $module" >>/tmp/wifi.log
    #    insmod $module
    #fi

    # bring up vifs
    for vif in $vifs; do
        config_get ifname $vif ifname
        config_get disabled $vif disabled
        echo "ifconfig $ifname down" >>/tmp/wifi.log
        if [ "$disabled" == "1" ]; then
            echo "$ifname marked disabled, skip" >>/tmp/wifi.log
            continue
        else
            echo "ifconfig $ifname up" >>/tmp/wifi.log
            ifconfig $ifname up
        fi
    done

    chk8021x $device
}

prepare_ralink_wifi() {
    echo "prepare_ralink_wifi($1,$2,$3,$4)" >>/tmp/wifi.log
    local device=$1
    config_get channel $device channel
    config_get ssid $2 ssid
    config_get mode $device mode
    config_get ht $device ht
    config_get country $device country
    config_get regdom $device regdom

    # HT40 mode can be enabled only in bgn (mode = 9), gn (mode = 7)
    # or n (mode = 6).
    HT=0
    [ "$mode" = 6 -o "$mode" = 7 -o "$mode" = 9 ] && [ "$ht" != "20" ] && HT=1

    # In HT40 mode, a second channel is used. If EXTCHA=0, the extra
    # channel is $channel + 4. If EXTCHA=1, the extra channel is
    # $channel - 4. If the channel is fixed to 1-4, we'll have to
    # use + 4, otherwise we can just use - 4.
    EXTCHA=0
    [ "$channel" != auto ] && [ "$channel" -lt "5" ] && EXTCHA=1
    
}

scan_ralink_wifi() {
    local device="$1"
    local module="$2"
    echo "scan_ralink_wifi($1,$2,$3,$4)" >>/tmp/wifi.log
    repair_wireless_uci
    sync_uci_with_dat $device /etc/wireless/$device/$device.dat
}

disable_ralink_wifi() {
    echo "disable_ralink_wifi($1,$2,$3,$4)" >>/tmp/wifi.log
    local device="$1"
    config_get vifs "$device" vifs
    for vif in $vifs; do
        config_get ifname $vif ifname
        ifconfig $ifname down
    done

    # kill any running ap_clients
    killall ap_client 2>/dev/null || true
}

enable_ralink_wifi() {
    echo "enable_ralink_wifi($1,$2,$3,$4)" >>/tmp/wifi.log
    local device="$1"
    config_get vifs "$device" vifs

    # bring up vifs
    for vif in $vifs; do
        config_get ifname $vif ifname
        config_get disabled $vif disabled
	config_get radio $device radio
        ifconfig $ifname down
        echo "ifconfig $ifname down" >>/dev/null
        if [ "$disabled" == "1" ]; then
            echo "$ifname marked disabled, skip" >>/dev/null
            continue
        else
            echo "ifconfig $ifname up" >>/dev/null
            ifconfig $ifname up
        fi
	#Radio On/Off only support iwpriv command but dat file
        [ "$radio" == "0" ] && iwpriv $ifname set RadioOn=0
        local net_cfg bridge
        net_cfg="$(find_net_config "$vif")"
        [ -z "$net_cfg" ] || {
            bridge="$(bridge_interface "$net_cfg")"
            config_set "$vif" bridge "$bridge"
            start_net "$ifname" "$net_cfg"
        }
	chk8021x $device
        set_wifi_up "$vif" "$ifname"
    done
}

detect_ralink_wifi() {
    echo "detect_ralink_wifi($1,$2,$3,$4)" >>/tmp/wifi.log
    local channel
    local device="$1"
    local module="$2"
    local band
    local ifname
    cd /sys/module/
    [ -d $module ] || return
    config_get channel $device channel
    [ -z "$channel" ] || return
    case "$device" in
        mt7620 | mt7602e | mt7603e | mt7628 )
            ifname="ra0"
            band="2.4G"
            ;;
        mt7610e | mt7612e )
            ifname="rai0"
            band="5G"
            ;;
        * )
            echo "device $device not recognized!! " >>/tmp/wifi.log
            ;;
    esac                    
    cat <<EOF
config wifi-device    $device
    option type     $device
    option vendor   ralink
    option band     $band
    option channel  0
    option autoch   2

config wifi-iface
    option device   $device
    option ifname    $ifname
    option network  lan
    option mode     ap
    option ssid OpenWrt-$device
    option encryption psk2
    option key      12345678

EOF
}



