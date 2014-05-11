#!/bin/sh
#
# by lintel@gmail.com, hoowa.sun@gmail.com
#

append DRIVERS "rt2860v2"

prepare_config() {
#获取参数 存储配置的变量 目标配置关键字

	local num=0 mode disabled
	
#准备产生RaX的无线配置
	local device=$1

#获取当前的无线频道
	config_get channel $device channel

#获取当前的802.11无线模式
	config_get hwmode $device mode
	
#获取WMM支持
	config_get wmm $device wmm
	
#获取设备的传输功率
	config_get txpower $device txpower
	
#获取设备的HT（频宽）
	config_get ht $device ht

#获取国家代码	
	config_get country $device country
	
#是否有MAC过滤
	config_get macpolicy $device macpolicy

#MAC地址过滤列表
	config_get maclist $device maclist
#字符格式转义
	ra_maclist="${maclist// /;};"
#是否支持GREEN AP功能
	config_get_bool greenap $device greenap 0

	config_get_bool antdiv "$device" diversity
	
	config_get frag "$device" frag 2346
	
	config_get rts "$device" rts 2347
	
	config_get distance "$device" distance

	config_get hidessid "$device" hidden 0
	
#获取该Radio下面的虚拟接口	
	config_get vifs "$device" vifs
	
#获取虚拟接口的数量，并提前配置SSID
for vif in $vifs; do
	let num+=1
	config_get_bool disabled "$vif" disabled 0
	config_get mode "$vif" mode 0
	
	#如果某个SSID接口需要隐藏，则所有的接口都隐藏
	[ "$hidessid" == "0" ] && {
	config_get hidessid $vif hidden 0
	}
	
	#已经关闭的接口以及sta模式的排除在外。
	[ "$mode" = "sta" ]&& {
	let num-=1 
	continue
	}
	[ "$disabled" == "1" ]&& {
	let num-=1
	continue
	}
	
	case $num in
	1)
		config_get ssid1 "$vif" ssid
		;;
	2)
		config_get ssid2 "$vif" ssid
		;;
	3)
		config_get ssid3 "$vif" ssid
		;;
	4)
		config_get ssid4 "$vif" ssid
		;;
	*)
		;;
	esac
done

#开始准备HT模式配置，注意HT模式仅在11N下有效。
	HT=1
	HT_CE=1

    if [ "$ht" = "20" ]; then
      HT=0 
    elif [ "$ht" = "20+40" ]; then
      HT=1 
      HT_CE=1
    elif [ "$ht" = "40" ] ; then
      HT=1 
      HT_CE=0
    else
    echo "ht config has some problem!use default!!!"
      HT=0
      HT_CE=1
    fi


	# 在HT40模式下,需要另外的一个频道，如果EXTCHA=0,则当前第二频道为$channel + 4.
	# 如果EXTCHA=1,则当前的第二频道为$channel - 4.
	# 如果当前频道被限制在1-4,则是当前频道+ 4，若否，则为当前频道-4 
	
	EXTCHA=1
	
	[ "$channel" != auto ] && [ "$channel" -lt "5" ] && EXTCHA=1

#配置自动选择无线频道
    [ "$channel" == "auto" ] && {
        channel=11
        AutoChannelSelect=2
    }

#开始判断WiFi的MAC过滤方式.
    case "$macpolicy" in
	allow|2)
	ra_macfilter=1;
	;;
	deny|1)
	ra_macfilter=2;
	;;
	*|disable|none|0)
	ra_macfilter=0;
	;;
    esac

	cat > /tmp/RT2860.dat<<EOF
#The word of "Default" must not be removed
Default
CountryRegion=0
CountryRegionABand=7
CountryCode=${country:-US}
BssidNum=${num:-1}
SSID1=${ssid1:-PandoraBox_SSID1}
SSID2=${ssid2:-PandoraBox_SSID2}
SSID3=${ssid3:-PandoraBox_SSID3}
SSID4=${ssid4:-PandoraBox_SSID4}
SSID5=
SSID6=
SSID7=
SSID8=
WirelessMode=${hwmode:-9}
FixedTxMode=
TxRate=0
Channel=${channel:-11}
BasicRate=15
BeaconPeriod=100
DtimPeriod=1
TxPower=${txpower:-100}
DisableOLBC=0
BGProtection=0
TxAntenna=
RxAntenna=
TxPreamble=1
RTSThreshold=${rts:-2347}
FragThreshold=${frag:-2346}
TxBurst=1
PktAggregate=1
AutoProvisionEn=0
FreqDelta=0
TurboRate=0
WmmCapable=${wmm:-0}
APAifsn=3;7;1;1
APCwmin=4;4;3;2
APCwmax=6;10;4;3
APTxop=0;0;94;47
APACM=0;0;0;0
BSSAifsn=3;7;2;2
BSSCwmin=4;4;3;2
BSSCwmax=10;10;4;3
BSSTxop=0;0;94;47
BSSACM=0;0;0;0
AckPolicy=0;0;0;0
APSDCapable=0
DLSCapable=0
NoForwarding=0
NoForwardingBTNBSSID=0
HideSSID=${hidessid:-0}
ShortSlot=1
AutoChannelSelect=${AutoChannelSelect:-0}
IEEE8021X=0
IEEE80211H=0
CarrierDetect=0
ITxBfEn=0
PreAntSwitch=
PhyRateLimit=0
DebugFlags=0
ETxBfEnCond=0
ITxBfTimeout=0
ETxBfTimeout=0
ETxBfNoncompress=0
ETxBfIncapable=0
FineAGC=0
StreamMode=0
StreamModeMac0=
StreamModeMac1=
StreamModeMac2=
StreamModeMac3=
CSPeriod=6
RDRegion=
StationKeepAlive=0
DfsLowerLimit=0
DfsUpperLimit=0
DfsOutdoor=0
SymRoundFromCfg=0
BusyIdleFromCfg=0
DfsRssiHighFromCfg=0
DfsRssiLowFromCfg=0
DFSParamFromConfig=0
FCCParamCh0=
FCCParamCh1=
FCCParamCh2=
FCCParamCh3=
CEParamCh0=
CEParamCh1=
CEParamCh2=
CEParamCh3=
JAPParamCh0=
JAPParamCh1=
JAPParamCh2=
JAPParamCh3=
JAPW53ParamCh0=
JAPW53ParamCh1=
JAPW53ParamCh2=
JAPW53ParamCh3=
FixDfsLimit=0
LongPulseRadarTh=0
AvgRssiReq=0
DFS_R66=0
BlockCh=
GreenAP=0
PreAuth=0
AuthMode=OPEN
EncrypType=NONE
WapiPsk1=0123456789
WapiPsk2=
WapiPsk3=
WapiPsk4=
WapiPsk5=
WapiPsk6=
WapiPsk7=
WapiPsk8=
WapiPskType=0
Wapiifname=
WapiAsCertPath=
WapiUserCertPath=
WapiAsIpAddr=
WapiAsPort=
RekeyMethod=DISABLE
RekeyInterval=3600
PMKCachePeriod=10
MeshAutoLink=0
MeshAuthMode=
MeshEncrypType=
MeshDefaultkey=0
MeshWEPKEY=
MeshWPAKEY=
MeshId=
WPAPSK1=12345678
WPAPSK2=
WPAPSK3=
WPAPSK4=
WPAPSK5=
WPAPSK6=
WPAPSK7=
WPAPSK8=
DefaultKeyID=1
Key1Type=0
Key1Str1=
Key1Str2=
Key1Str3=
Key1Str4=
Key1Str5=
Key1Str6=
Key1Str7=
Key1Str8=
Key2Type=0
Key2Str1=
Key2Str2=
Key2Str3=
Key2Str4=
Key2Str5=
Key2Str6=
Key2Str7=
Key2Str8=
Key3Type=0
Key3Str1=
Key3Str2=
Key3Str3=
Key3Str4=
Key3Str5=
Key3Str6=
Key3Str7=
Key3Str8=
Key4Type=0
Key4Str1=
Key4Str2=
Key4Str3=
Key4Str4=
Key4Str5=
Key4Str6=
Key4Str7=
Key4Str8=
HSCounter=0
HT_HTC=1
HT_RDG=1
HT_LinkAdapt=0
HT_OpMode=0
HT_MpduDensity=5
HT_EXTCHA=${EXTCHA}
HT_BW=${HT:-0}
HT_AutoBA=1
HT_BADecline=0
HT_AMSDU=0
HT_BAWinSize=64
HT_GI=1
HT_STBC=1
HT_MCS=33
HT_TxStream=2
HT_RxStream=2
HT_PROTECT=1
HT_DisallowTKIP=1
HT_BSSCoexistence=${HT_CE:-1}
GreenAP=${greenap:-0}
WscConfMode=0
WscConfStatus=1
WCNTest=0
AccessPolicy0=${ra_macfilter:-0}
AccessControlList0=${ra_maclist:-0}
AccessPolicy1=0
AccessControlList1=
AccessPolicy2=0
AccessControlList2=
AccessPolicy3=0
AccessControlList3=
AccessPolicy4=0
AccessControlList4=
AccessPolicy5=0
AccessControlList5=
AccessPolicy6=0
AccessControlList6=
AccessPolicy7=0
AccessControlList7=
WdsEnable=0
WdsPhyMode=
WdsEncrypType=NONE
WdsList=
Wds0Key=
Wds1Key=
Wds2Key=
Wds3Key=
RADIUS_Server=
RADIUS_Port=1812
RADIUS_Key1=
RADIUS_Key2=
RADIUS_Key3=
RADIUS_Key4=
RADIUS_Key5=
RADIUS_Key6=
RADIUS_Key7=
RADIUS_Key8=
RADIUS_Acct_Server=
RADIUS_Acct_Port=1813
RADIUS_Acct_Key=
own_ip_addr=
Ethifname=
EAPifname=
PreAuthifname=
session_timeout_interval=0
idle_timeout_interval=0
WiFiTest=0
TGnWifiTest=0
ApCliEnable=0
ApCliSsid=
ApCliBssid=
ApCliAuthMode=
ApCliEncrypType=
ApCliWPAPSK=
ApCliDefaultKeyID=0
ApCliKey1Type=0
ApCliKey1Str=
ApCliKey2Type=0
ApCliKey2Str=
ApCliKey3Type=0
ApCliKey3Str=
ApCliKey4Type=0
ApCliKey4Str=
RadioOn=1
SSID=
WPAPSK=0123456789
Key1Str=
Key2Str=
Key3Str=
Key4Str=
EOF

}

reload_rt2860v2() {
	ifconfig ra0 down
	rmmod rt2860v2_ap 

	insmod rt2860v2_ap
	ifconfig ra0 up
}

scan_rt2860v2() {
	local device="$1"
}

disable_rt2860v2() {
	local device="$1"
	set_wifi_down $device
	ifconfig $device down
	true
}

rt2860v2_start_vif() {
	local vif="$1"
	local ifname="$2"

	local net_cfg
	net_cfg="$(find_net_config "$vif")"
	[ -z "$net_cfg" ] || start_net "$ifname" "$net_cfg"

	set_wifi_up "$vif" "$ifname"
}

enable_rt2860v2() {
#传参过来的第一个参数是Radio0
	local device="$1" dmode if_num=0;
	
	config_get_bool disabled "$device" disabled 0	
	if [ "$disabled" = "1" ] ;then
	ifconfig ra0 down
	return
	fi
	
	#开始准备该设备的无线配置参数
	prepare_config $device
	
	config_get dmode $device mode
	config_get vifs "$device" vifs

	config_get maxassoc $device maxassoc 0

	for vif in $vifs; do
		local ifname encryption key ssid mode
		
		config_get ifname $vif device	
		
		#根据ifname数量配置多SSID
		[ "$ifname" == "ra0" ] && {
		ifname="ra$if_num"
		}
		let if_num+=1
		#排除如果设置为apcli0
		[ "$mode" = "sta" ]&& {let if_num-=1}
		
		config_get_bool disabled $vif disabled 0
		if [ "$disabled" = "1" ] ;then
		set_wifi_down $ifname
		echo "Interface $ifname disabled"
		return
		fi
		config_get encryption $vif encryption
		config_get key $vif key
		config_get ssid $vif ssid
		config_get mode $vif mode
		config_get wps $vif wps
		#是否隔离客户端
		config_get isolate $vif isolate 0
		#802.11h
		config_get doth $vif doth 0
		#是否隐藏SSID
#		config_get hidessid $vif hidden 0

		#STA APClient配置
		[ "$mode" == "sta" ] && {
					#如果为apcli模式，指定接口名称为apcli0
					ifname="apcli0"
					echo "#Encryption" >/tmp/wifi_encryption_${ifname}.dat
					ifconfig $ifname down
					iwpriv $ifname set ApCliEnable=0 
					iwpriv $ifname set ApCliSsid=$ssid
					config_get bssid $vif bssid 0
					[ -z "$mode" ] && {
					iwpriv $ifname set ApCliBssid=$bssid
					echo "APCli use bssid connect."
					}
			case "$encryption" in
				none)
					echo "NONE" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set ApCliAuthMode=OPEN 
					iwpriv $ifname set ApCliEncrypType=NONE 
					;;
				WEP|wep|wep-open)
					echo "WEP" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set AuthMode=WEPAUTO
					iwpriv $ifname set ApCliEncrypType=WEP
					iwpriv $ifname set Key0=${key}
					;;
				WEP-SHARE|wep-shared)
					echo "WEP" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set AuthMode=WEPAUTO
					iwpriv $ifname set ApCliEncrypType=WEP
					iwpriv $ifname set Key0=${key}
					;;
				WPA*|wpa*|WPA2-PSK|psk*)
					echo "WPA2" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set ApCliAuthMode=WPAPSKWPA2PSK
					iwpriv $ifname set ApCliEncrypType=AES
					iwpriv $ifname set ApCliWPAPSK=$key
					echo "WPAPSKWPA2PSK" >>/tmp/wifi_encryption_${ifname}.dat
					echo "TKIPAES" >>/tmp/wifi_encryption_${ifname}.dat
					;;
			esac
					iwpriv $ifname set ApCliEnable=1
					ifconfig $ifname up
		}
		#AP模式配置
		[ "$mode" == "sta" ] || {
			[ "$key" = "" -a "$vif" = "private" ] && {
				logger "no key set serial"
				key="AAAAAAAAAA"
			}
			[ "$dmode" == "6" ] && wpa_crypto="aes"
			ifconfig $ifname up
			#判断当前加密模式
			echo "#Encryption" >/tmp/wifi_encryption_${ifname}.dat
			iwpriv $ifname set "SSID=${ssid}"
			case "$encryption" in
				#找到WPA/WPA2加密
				wpa*|psk*|WPA*|Mixed|mixed)
					echo "WPA" >>/tmp/wifi_encryption_${ifname}.dat
					local enc
					case "$encryption" in
						Mixed|mixed|psk+psk2)
							enc=WPAPSKWPA2PSK
							;;
						WPA2*|wpa2*|psk2*)
							enc=WPA2PSK
							;;
						WPA*|WPA1*|wpa*|wpa1*|psk*)
							enc=WPAPSK
							;;
					esac
					local crypto="AES"
					case "$encryption" in
					*tkip+aes*|*tkip+ccmp*|*aes+tkip*|*ccmp+tkip*)
						crypto="TKIPAES"
						;;
					*aes*|*ccmp*)
						crypto="AES"
						;;
					*tkip*) 
						crypto="TKIP"
						echo Warring!!! TKIP not support in 802.11n 40Mhz!!!
					;;
					esac
					echo "$enc" >>/tmp/wifi_encryption_${ifname}.dat
					echo "$crypto" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set AuthMode=$enc
					iwpriv $ifname set EncrypType=$crypto
					iwpriv $ifname set IEEE8021X=0
					iwpriv $ifname set "SSID=${ssid}"
					iwpriv $ifname set "WPAPSK=${key}"
					iwpriv $ifname set DefaultKeyID=2
					iwpriv $ifname set "SSID=${ssid}"
						
					if [ "DefaultKeyID=2$wps" == "1" ]; then
						iw"SSID=${ssid}"priv $ifname set WscConfMode=7
					else
						iwpriv $ifname set WscConfMode=0
					fi
					;;
				WEP|wep|wep-open|wep-shared)
					echo "WEP" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set AuthMode=WEPAUTO
					iwpriv $ifname set EncrypType=WEP
					iwpriv $ifname set IEEE8021X=0
					for idx in 1 2 3 4; do
						config_get keyn $vif key${idx}
						[ -n "$keyn" ] && iwpriv $ifname set "Key${idx}=${keyn}"
					done
					iwpriv $ifname set DefaultKeyID=${key}
					iwpriv $ifname set "SSID=${ssid}"
					echo 
					iwpriv $ifname set WscConfMode=0
					;;
				none|open)
					echo "NONE" >>/tmp/wifi_encryption_${ifname}.dat
					iwpriv $ifname set AuthMode=OPEN
					iwpriv $ifname set WscConfMode=0
					iwpriv $ifname set EncrypType=NONE
					;;
			esac
		}
		
		#如果关闭了WIFI，则关闭RF
		if [ $disabled == 1 ]; then
		 iwpriv $ifname set RadioOn=0
		 set_wifi_down $ifname
		else
		 iwpriv $ifname set RadioOn=1
		fi

		#检查是否需要进行SSID隐藏。
#		if [ $hidessid == "1" ]; then
#		 iwpriv $ifname set HideSSID=1
#		else
#		 iwpriv $ifname set HideSSID=0
#		fi

		#隔离客户端连接。
		[ $isolate == "1" ]&&{
			iwpriv $ifname set NoForwarding=1
		}
		
		#802.11h 支持
		[ $doth == "1" ]&&{
			iwpriv $ifname set IEEE80211H=1
		}	
		
		ifconfig "$ifname" up
		if [ "$mode" == "sta" ];then {
			net_cfg="$(find_net_config "$vif")"
			[ -z "$net_cfg" ] || {
					rt2860v2_start_vif "$vif" "$ifname"

			}
		}
		else
		{
			local net_cfg bridge
			net_cfg="$(find_net_config "$vif")"
			[ -z "$net_cfg" ]||{
				bridge="$(bridge_interface "$net_cfg")"
				config_set "$vif" bridge "$bridge"
				rt2860v2_start_vif "$vif" "$ifname"
				#Fix bridge problem
				[ -z `brctl show |grep $ifname` ] && {
				brctl addif $(bridge_interface "$net_cfg") $ifname
				}
				
			}



		}
		fi;
		set_wifi_up "$vif" "$ifname"

		# If isolation is requested, disable forwarding between
		# wireless clients (both within the same BSSID and
		# between BSSID's, though the latter is probably not
		# relevant for our setup).
		
#		iwpriv $ifname set NoForwarding="${isolate:-0}"
#		iwpriv $ifname set NoForwardingBTNBSSID="${isolate:-0}"

	done
	
	#配置无线最大连接数
	iwpriv $device set MaxStaNum=$maxassoc
}

first_enable() {

ifconfig ra0 down

	cat > /tmp/RT2860.dat<<EOF
#The word of "Default" must not be removed
Default
CountryRegion=0
CountryRegionABand=7
CountryCode=US
BssidNum=1
SSID1=PandoraBox
WirelessMode=9
FixedTxMode=
TxRate=0
Channel=11
BasicRate=15
BeaconPeriod=100
DtimPeriod=1
TxPower=100
DisableOLBC=0
BGProtection=0
TxAntenna=
RxAntenna=
TxPreamble=1
RTSThreshold=2347
FragThreshold=2346
TxBurst=1
PktAggregate=1
AutoProvisionEn=0
FreqDelta=0
TurboRate=0
WmmCapable=1
APAifsn=3;7;1;1
APCwmin=4;4;3;2
APCwmax=6;10;4;3
APTxop=0;0;94;47
APACM=0;0;0;0
BSSAifsn=3;7;2;2
BSSCwmin=4;4;3;2
BSSCwmax=10;10;4;3
BSSTxop=0;0;94;47
BSSACM=0;0;0;0
AckPolicy=0;0;0;0
APSDCapable=0
DLSCapable=0
NoForwarding=0
NoForwardingBTNBSSID=0
HideSSID=1
ShortSlot=1
AutoChannelSelect=0
IEEE8021X=0
IEEE80211H=0
CarrierDetect=0
ITxBfEn=0
PreAntSwitch=
PhyRateLimit=0
DebugFlags=0
ETxBfEnCond=0
ITxBfTimeout=0
ETxBfTimeout=0
ETxBfNoncompress=0
ETxBfIncapable=0
FineAGC=0
StreamMode=0
StreamModeMac0=
StreamModeMac1=
StreamModeMac2=
StreamModeMac3=
CSPeriod=6
RDRegion=
StationKeepAlive=0
DfsLowerLimit=0
DfsUpperLimit=0
DfsOutdoor=0
SymRoundFromCfg=0
BusyIdleFromCfg=0
DfsRssiHighFromCfg=0
DfsRssiLowFromCfg=0
DFSParamFromConfig=0
FCCParamCh0=
FCCParamCh1=
FCCParamCh2=
FCCParamCh3=
CEParamCh0=
CEParamCh1=
CEParamCh2=
CEParamCh3=
JAPParamCh0=
JAPParamCh1=
JAPParamCh2=
JAPParamCh3=
JAPW53ParamCh0=
JAPW53ParamCh1=
JAPW53ParamCh2=
JAPW53ParamCh3=
FixDfsLimit=0
LongPulseRadarTh=0
AvgRssiReq=0
DFS_R66=0
BlockCh=
GreenAP=0
PreAuth=0
AuthMode=OPEN
EncrypType=NONE
WapiPsk1=0123456789
WapiPsk2=
WapiPsk3=
WapiPsk4=
WapiPsk5=
WapiPsk6=
WapiPsk7=
WapiPsk8=
WapiPskType=0
Wapiifname=
WapiAsCertPath=
WapiUserCertPath=
WapiAsIpAddr=
WapiAsPort=
RekeyMethod=DISABLE
RekeyInterval=3600
PMKCachePeriod=10
MeshAutoLink=0
MeshAuthMode=
MeshEncrypType=
MeshDefaultkey=0
MeshWEPKEY=
MeshWPAKEY=
MeshId=
WPAPSK1=12345678
WPAPSK2=
WPAPSK3=
WPAPSK4=
WPAPSK5=
WPAPSK6=
WPAPSK7=
WPAPSK8=
DefaultKeyID=1
Key1Type=0
Key1Str1=
Key1Str2=
Key1Str3=
Key1Str4=
Key1Str5=
Key1Str6=
Key1Str7=
Key1Str8=
Key2Type=0
Key2Str1=
Key2Str2=
Key2Str3=
Key2Str4=
Key2Str5=
Key2Str6=
Key2Str7=
Key2Str8=
Key3Type=0
Key3Str1=
Key3Str2=
Key3Str3=
Key3Str4=
Key3Str5=
Key3Str6=
Key3Str7=
Key3Str8=
Key4Type=0
Key4Str1=
Key4Str2=
Key4Str3=
Key4Str4=
Key4Str5=
Key4Str6=
Key4Str7=
Key4Str8=
HSCounter=0
HT_HTC=1
HT_RDG=1
HT_LinkAdapt=0
HT_OpMode=0
HT_MpduDensity=5
HT_EXTCHA=0
HT_BW=0
HT_AutoBA=1
HT_BADecline=0
HT_AMSDU=0
HT_BAWinSize=64
HT_GI=1
HT_STBC=1
HT_MCS=33
HT_TxStream=2
HT_RxStream=2
HT_PROTECT=1
HT_DisallowTKIP=1
HT_BSSCoexistence=1
GreenAP=0
WscConfMode=0
WscConfStatus=1
WCNTest=0
AccessPolicy0=0
AccessControlList0=
AccessPolicy1=0
AccessControlList1=
AccessPolicy2=0
AccessControlList2=
AccessPolicy3=0
AccessControlList3=
AccessPolicy4=0
AccessControlList4=
AccessPolicy5=0
AccessControlList5=
AccessPolicy6=0
AccessControlList6=
AccessPolicy7=0
AccessControlList7=
WdsEnable=0
WdsPhyMode=
WdsEncrypType=NONE
WdsList=
Wds0Key=
Wds1Key=
Wds2Key=
Wds3Key=
RADIUS_Server=
RADIUS_Port=1812
RADIUS_Key1=
RADIUS_Key2=
RADIUS_Key3=
RADIUS_Key4=
RADIUS_Key5=
RADIUS_Key6=
RADIUS_Key7=
RADIUS_Key8=
RADIUS_Acct_Server=
RADIUS_Acct_Port=1813
RADIUS_Acct_Key=
own_ip_addr=
Ethifname=
EAPifname=
PreAuthifname=
session_timeout_interval=0
idle_timeout_interval=0
WiFiTest=0
TGnWifiTest=0
ApCliEnable=0
ApCliSsid=
ApCliBssid=
ApCliAuthMode=
ApCliEncrypType=
ApCliWPAPSK=
ApCliDefaultKeyID=0
ApCliKey1Type=0
ApCliKey1Str=
ApCliKey2Type=0
ApCliKey2Str=
ApCliKey3Type=0
ApCliKey3Str=
ApCliKey4Type=0
ApCliKey4Str=
RadioOn=1
SSID=
WPAPSK=0123456789
Key1Str=
Key2Str=
Key3Str=
Key4Str=
EOF

ifconfig ra0 up
}

#detect_rt2860v2函数用于检测是否存在驱动
detect_rt2860v2() {
	local i=-1
#判断系统是否存在rt2860v2_ap，不存在则退出
	cd /sys/module/
	[ -d rt2860v2_ap ] || return
#检测系统存在多少个wifi接口
	while grep -qs "^ *ra$((++i)):" /proc/net/dev; do
		config_get type ra${i} type
		[ "$type" = rt2860v2 ] && continue
		
#检查并创建WiFi驱动配置链接
	[ -f /etc/Wireless/RT2860/RT2860.dat ] || {
	mkdir -p /etc/Wireless/RT2860/
	ln -s /tmp/RT2860.dat /etc/Wireless/RT2860/RT2860.dat
	}
	
	first_enable
	
		cat <<EOF
config wifi-device  ra${i}
	option type     rt2860v2
	option mode 	9
	option channel  auto
	option txpower 100
	option ht 20+40
    	option country US
# REMOVE THIS LINE TO ENABLE WIFI:
	option disabled 0	
	
config wifi-iface
	option device   ra${i}
	option network	lan
	option mode     ap
	option ssid     PandoraBox${i#0}_$(cat /sys/class/net/ra${i}/address|awk -F ":" '{print $4""$5""$6 }'| tr a-z A-Z)
	option encryption none
EOF

	ifconfig ra0 down 
	done
	
}


