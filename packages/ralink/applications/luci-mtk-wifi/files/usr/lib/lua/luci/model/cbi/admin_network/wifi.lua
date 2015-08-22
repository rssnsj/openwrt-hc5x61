--[[
LuCI - Lua Configuration Interface

Copyright 2008 Steven Barth <steven@midlink.org>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

$Id$
]]--

local wa = require "luci.tools.webadmin"
local nw = require "luci.model.network"
local ut = require "luci.util"
local nt = require "luci.sys".net
local fs = require "nixio.fs"

arg[1] = arg[1] or ""

m = Map("wireless", "",
	translate("The <em>Device Configuration</em> section covers physical settings of the radio " ..
		"hardware such as channel, transmit power or antenna selection which are shared among all " ..
		"defined wireless networks (if the radio hardware is multi-SSID capable). Per network settings " ..
		"like encryption or operation mode are grouped in the <em>Interface Configuration</em>."))

m:chain("network")
m:chain("firewall")

local ifsection

function m.on_commit(map)
	local wnet = nw:get_wifinet(arg[1])
	if ifsection and wnet then
		ifsection.section = wnet.sid
		m.title = luci.util.pcdata(wnet:get_i18n())
	end
end

nw.init(m.uci)

local wnet = nw:get_wifinet(arg[1])
local wdev = wnet and wnet:get_device()
local vendor = wdev:get("vendor")

-- redirect to overview page if network does not exist anymore (e.g. after a revert)
if not wnet or not wdev then
	luci.http.redirect(luci.dispatcher.build_url("admin/network/wireless"))
	return
end

-- wireless toggle was requested, commit and reload page
function m.parse(map)
	if m:formvalue("cbid.wireless.%s.__toggle" % wdev:name()) then
		if wdev:get("disabled") == "1" or wnet:get("disabled") == "1" then
			wnet:set("disabled", nil)
		else
			wnet:set("disabled", "1")
		end
		wdev:set("disabled", nil)

		nw:commit("wireless")
		if vendor == "ralink" then
			luci.sys.call("(env -i /sbin/wifi down; env -i /sbin/wifi up) >/dev/null 2>/dev/null")
		else
			luci.sys.call("(env -i /bin/ubus call network reload) >/dev/null 2>/dev/null")
		end

		luci.http.redirect(luci.dispatcher.build_url("admin/network/wireless", arg[1]))
		return
	end
	Map.parse(map)
end

m.title = luci.util.pcdata(wnet:get_i18n())


local function txpower_list(iw)
	local list = iw.txpwrlist or { }
	local off  = tonumber(iw.txpower_offset) or 0
	local new  = { }
	local prev = -1
	local _, val
	for _, val in ipairs(list) do
		local dbm = val.dbm + off
		local mw  = math.floor(10 ^ (dbm / 10))
		if mw ~= prev then
			prev = mw
			new[#new+1] = {
				display_dbm = dbm,
				display_mw  = mw,
				driver_dbm  = val.dbm,
				driver_mw   = val.mw
			}
		end
	end
	return new
end

local function txpower_current(pwr, list)
	pwr = tonumber(pwr)
	if pwr ~= nil then
		local _, item
		for _, item in ipairs(list) do
			if item.driver_dbm >= pwr then
				return item.driver_dbm
			end
		end
	end
	return (list[#list] and list[#list].driver_dbm) or pwr or 0
end

local iw = luci.sys.wifi.getiwinfo(arg[1])
local hw_modes      = iw.hwmodelist or { }
local tx_power_list = txpower_list(iw)
local tx_power_cur  = txpower_current(wdev:get("txpower"), tx_power_list)

s = m:section(NamedSection, wdev:name(), "wifi-device", translate("Device Configuration"))
s.addremove = false

s:tab("general", translate("General Setup"))
s:tab("macfilter", translate("MAC-Filter"))
s:tab("advanced", translate("Advanced Settings"))
if vendor == "ralink" then
	s:tab("htmode", translate("HT Physical Mode"))
end
--[[
back = s:option(DummyValue, "_overview", translate("Overview"))
back.value = ""
back.titleref = luci.dispatcher.build_url("admin", "network", "wireless")
]]

st = s:taboption("general", DummyValue, "__status", translate("Status"))
st.template = "admin_network/wifi_status"
st.ifname   = arg[1]

if vendor == "ralink" then
	radio = s:taboption("general", ListValue, "radio", translate("Radio on/off"))
	radio:value("1",translate("on"))
	radio:value("0",translate("off"))
	radio.default = 1

	local band = wdev:get("band")
	local ifname = wnet:ifname()
else
	en = s:taboption("general", Button, "__toggle")

	if wdev:get("disabled") == "1" or wnet:get("disabled") == "1" then
		en.title      = translate("Wireless network is disabled")
		en.inputtitle = translate("Enable")
		en.inputstyle = "apply"
	else
		en.title      = translate("Wireless network is enabled")
		en.inputtitle = translate("Disable")
		en.inputstyle = "reset"
	end
end

local hwtype = wdev:get("type")

-- NanoFoo
local nsantenna = wdev:get("antenna")

-- Check whether there is a client interface on the same radio,
-- if yes, lock the channel choice as the station will dicatate the freq
local has_sta = nil
local _, net
for _, net in ipairs(wdev:get_wifinets()) do
	if net:mode() == "sta" and net:id() ~= wnet:id() then
		has_sta = net
		break
	end
end

if vendor ~= "ralink" then
	if has_sta then
		ch = s:taboption("general", DummyValue, "choice", translate("Channel"))
		ch.value = translatef("Locked to channel %d used by %s",
			has_sta:channel(), has_sta:shortname())
	else
		ch = s:taboption("general", Value, "channel", translate("Channel"))
		ch:value("auto", translate("auto"))
	        for _, f in ipairs(iw and iw.freqlist or { }) do
			if not f.restricted then
				ch:value(f.channel, "%i (%.3f GHz)" %{ f.channel, f.mhz / 1000 })
			end
		end
	end
end

----------------- MTK Device ------------------

if vendor == "ralink" then
	wifimode = s:taboption("general", ListValue, "wifimode", translate("Network Mode"))
        wifimode:value("0", translate("802.11b/g"))
        wifimode:value("1", translate("802.11b"))
        wifimode:value("2", translate("802.11a"))
        wifimode:value("4", translate("802.11g"))
        wifimode:value("7", translate("802.11g/n"))
        wifimode:value("8", translate("802.11a/n"))
        wifimode:value("9", translate("802.11b/g/n"))
        wifimode:value("14", translate("802.11a/an/ac"))
	wifimode:value("15", translate("802.11an/ac"))
       	if band == "5G" then
	wifimode.default = 14
	else
	wifimode.default = 9
	end
	ch = s:taboption("general", ListValue, "channel", translate("Channel"))
	ch:value("0","auto")
	if band == "5G" then
	ch:value("36","5.180 GHz (Channel 36)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="6"},{aregion="7"},{aregion="9"},{aregion="10"},{aregion="11"})
        ch:value("40","5.200 GHz (Channel 40)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="6"},{aregion="7"},{aregion="9"},{aregion="10"},{aregion="11"})
        ch:value("44","5.220 GHz (Channel 44)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="6"},{aregion="7"},{aregion="9"},{aregion="10"},{aregion="11"})
        ch:value("48","5.240 GHz (Channel 48)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="6"},{aregion="7"},{aregion="9"},{aregion="10"},{aregion="11"})
        ch:value("52","5.260 GHz (Channel 52)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="3"},{aregion="7"},{aregion="8"},{aregion="9"},{aregion="11"})
        ch:value("56","5.280 GHz (Channel 56)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="3"},{aregion="7"},{aregion="8"},{aregion="9"},{aregion="11"})
        ch:value("60","5.300 GHz (Channel 60)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="3"},{aregion="7"},{aregion="8"},{aregion="9"},{aregion="11"})
        ch:value("64","5.320 GHz (Channel 64)",{aregion="0"},{aregion="1"},{aregion="2"},{aregion="3"},{aregion="7"},{aregion="8"},{aregion="9"},{aregion="11"})
        ch:value("100","5.500 GHz (Channel 100)",{aregion="1"},{aregion="7"},{aregion="9"},{aregion="11"})
        ch:value("104","5.520 GHz (Channel 104)",{aregion="1"},{aregion="7"},{aregion="9"},{aregion="11"})
        ch:value("108","5.540 GHz (Channel 108)",{aregion="1"},{aregion="7"},{aregion="9"},{aregion="11"})
        ch:value("112","5.560 GHz (Channel 112)",{aregion="1"},{aregion="7"},{aregion="9"},{aregion="11"})
        ch:value("116","5.580 GHz (Channel 116)",{aregion="1"},{aregion="7"},{aregion="9"},{aregion="11"})
	ch:value("120","5.600 GHz (Channel 120)",{aregion="1"},{aregion="7"},{aregion="11"})
        ch:value("124","5.620 GHz (Channel 124)",{aregion="1"},{aregion="7"})
        ch:value("128","5.640 GHz (Channel 128)",{aregion="1"},{aregion="7"})
        ch:value("132","5.660 GHz (Channel 132)",{aregion="1"},{aregion="7"},{aregion="9"})
        ch:value("136","5.680 GHz (Channel 136)",{aregion="1"},{aregion="7"},{aregion="9"})
        ch:value("140","5.700 GHz (Channel 140)",{aregion="1"},{aregion="7"},{aregion="9"})
        ch:value("149","5.745 GHz (Channel 149)",{aregion="0"},{aregion="3"},{aregion="4"},{aregion="5"},{aregion="7"},{aregion="9"},{aregion="10"})
        ch:value("153","5.765 GHz (Channel 153)",{aregion="0"},{aregion="3"},{aregion="4"},{aregion="5"},{aregion="7"},{aregion="9"},{aregion="10"})
        ch:value("157","5.785 GHz (Channel 157)",{aregion="0"},{aregion="3"},{aregion="4"},{aregion="5"},{aregion="7"},{aregion="9"},{aregion="10"})
        ch:value("161","5.805 GHz (Channel 161)",{aregion="0"},{aregion="3"},{aregion="4"},{aregion="5"},{aregion="7"},{aregion="9"},{aregion="10"})
        ch:value("165","5.825 GHz (Channel 165)",{aregion="0"},{aregion="4"},{aregion="7"},{aregion="9"})
	else
	ch:value("1","2412MHz (Channel 1)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"})
        ch:value("2","2417MHz (Channel 2)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"})
        ch:value("3","2422MHz (Channel 3)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"})
        ch:value("4","2427MHz (Channel 4)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"})
        ch:value("5","2432MHz (Channel 5)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"},{region="7"})
        ch:value("6","2437MHz (Channel 6)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"},{region="7"})
        ch:value("7","2442MHz (Channel 7)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"},{region="7"})
        ch:value("8","2447MHz (Channel 8)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"},{region="7"})
        ch:value("9","2452MHz (Channel 9)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="6"},{region="7"})
        ch:value("10","2457MHz (Channel 10)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="2"},{region="3"},{region="7"})
        ch:value("11","2462MHz (Channel 11)",{region="0"},{region="1"},{region="5"},{region="31"},{region="32"},{region="33"},{region="2"},{region="3"},{region="7"})
        ch:value("12","2467MHz (Channel 12)",{region="7"},{region="1"},{region="5"},{region="31"},{region="33"},{region="3"})
        ch:value("13","2472MHz (Channel 13)",{region="7"},{region="1"},{region="5"},{region="31"},{region="33"},{region="3"})
        ch:value("14","2484MHz (Channel 14)",{region="4"},{region="5"},{region="31"},{region="33"})
	end
	bw = s:taboption("general", ListValue, "bw", translate("Band Width"))
	bw:value("0", translate("20MHz"))
        bw:value("1", translate("40MHz"))
	if band == "5G" then
        bw:value("2", translate("80MHz"))
	bw.default = 2
	else
	bw.default = 1
	end

	country = s:taboption("advanced", ListValue, "country", translate("Country Code"))
	country:value("US")
	country:value("JP")
	country:value("FR")
	country:value("TW")
	country:value("IE")
	country:value("HK")
	country:value("None")
	country.default = "None"

	region = s:taboption("advanced", ListValue, "region", translate("Support Channel"))
	region:depends("wifimode","0")
	region:depends("wifimode","1")
	region:depends("wifimode","4")
	region:depends("wifimode","7")
	region:depends("wifimode","9")
	region:value("0","CH1~11")
	region:value("1","CH1~13")
	region:value("2","CH10~11")
	region:value("3","CH10~13")
	region:value("4","CH13")
	region:value("5","CH1~14")
	region:value("6","CH3~9")
	region:value("7","CH5~13")
	region.default = 5
	aregion = s:taboption("advanced", ListValue, "aregion", translate("Support Channel"))
	aregion:depends("wifimode","2")
	aregion:depends("wifimode","6")
	aregion:depends("wifimode","14")
	aregion:depends("wifimode","15")
	aregion:value("0","Ch36~64, Ch149~165")
	aregion:value("1","Ch36~64, Ch100~140")
	aregion:value("2","Ch36~64")
	aregion:value("3","Ch52~64, Ch149~161")
	aregion:value("4","Ch149~165")
	aregion:value("5","Ch149~161")
	aregion:value("6","Ch36~48")
	aregion:value("7","Ch36~64, Ch100~140, Ch149~165")
	aregion:value("8","Ch52~64")
	aregion:value("9","Ch36~64, Ch100~116, Ch132~140, Ch149~165")
	aregion:value("10","Ch36~48, Ch149~165")
	aregion:value("11","Ch36~64, Ch100~120, Ch149~161")
	aregion:value("12","Ch149~161")
	aregion:value("13","Ch52~64, Ch100~140, Ch149~161")
	aregion:value("14","Ch36~64, Ch100~116, Ch136~140, Ch149~165")
	aregion:value("15","Ch149~165")
	aregion:value("16","Ch52~64, Ch149~165")
	aregion:value("17","Ch36~48, Ch149~161")
	aregion:value("18","Ch36~64, Ch100~116, Ch132~140")
	aregion:value("19","Ch56~64, Ch100~140, Ch149~161")
	aregion:value("20","Ch36~64, Ch100~124, Ch149~161")
	aregion:value("21","Ch36~64, Ch100~140, Ch149~161")
	aregion.default = 7
	bgprotect = s:taboption("advanced", ListValue, "bgprotect", translate("BG Protection Mode"))
        bgprotect:value("0",translate("auto"))
        bgprotect:value("1",translate("on"))
        bgprotect:value("2",translate("off"))
	BeaconPeriod = s:taboption("advanced", Value, "beacon", translate("Beacon Interval"))
	BeaconPeriod.datatype = "range(20,999)"
	BeaconPeriod.default = 100
	DTIM = s:taboption("advanced", Value, "dtim", translate("Data Beacon Rate"))
        DTIM.datatype = "range(1,255)"
	DTIM.default = 1
        FM = s:taboption("advanced", Value, "fragthres", translate("Fragment Threshold"))
        FM.datatype = "range(255,2346)"
	FM.default = 2346
        RTS= s:taboption("advanced", Value, "rtsthres", translate("RTS Threshold"))
        RTS.datatype = "range(1,2347)"
	RTS.default = 2347
        txpower = s:taboption("advanced", Value, "txpower", translate("TX Power"))
        txpower.datatype = "range(1,100)"
	txpower.default = 100
	ShortPreamble = s:taboption("advanced", ListValue, "txpreamble", translate("Short Preamble"))
        ShortPreamble:value("0",translate("Disable"))
        ShortPreamble:value("1",translate("Enable"))
	ShortPreamble.default = 0
        ShortSlot = s:taboption("advanced", ListValue, "shortslot", translate("Short Slot"))
        ShortSlot:value("0",translate("Disable"))
        ShortSlot:value("1",translate("Enable"))
	ShortSlot.default = 1
        TxBurst = s:taboption("advanced", ListValue, "txburst", translate("Tx Burst"))
        TxBurst:value("0",translate("Disable"))
        TxBurst:value("1",translate("Enable"))
	TxBurst.default = 1
        Pkt_Aggregate = s:taboption("advanced", ListValue, "pktaggre", translate("Pkt_Aggregate"))
        Pkt_Aggregate:value("0",translate("Disable"))
        Pkt_Aggregate:value("1",translate("Enable"))
	Pkt_Aggregate.default = 1
        IEEE80211H = s:taboption("advanced", ListValue, "ieee80211h", translate("IEEE 802.11H Support"))
        IEEE80211H:value("0",translate("Disable"))
        IEEE80211H:value("1",translate("Enable"))
	IEEE80211H.default = 0
--------------------------------------------HT Physical Mode-------------------------------------------------
	ht_bsscoexist = s:taboption("htmode", ListValue, "ht_bsscoexist", translate("20/40 Coexistence"))
	ht_bsscoexist:depends({wifimode="7",bw="1"})
	ht_bsscoexist:depends({wifimode="9",bw="1"})
	ht_bsscoexist:value("0","Disable")
	ht_bsscoexist:value("1","Enable")
	ht_extcha = s:taboption("htmode", ListValue, "ht_extcha", translate("Extension Channle"))
	ht_extcha:depends({wifimode="7",bw="1"})
	ht_extcha:depends({wifimode="9",bw="1"})
	ht_extcha:value("1","Above",{channel="0"},{channel="1"},{channel="2"},{channel="3"},{channel="4"},{channel="5"},{channel="6"},{channel="7"},{channel="8"},{channel="9"},{channel="10"})
	ht_extcha:value("0","Below",{channel="0"},{channle="5"},{channel="6"},{channel="7"},{channel="8"},{channel="9"},{channel="10"},{channel="11"},{channel="12"},{channel="13"})htopmode = s:taboption("htmode", ListValue, "ht_opmode", translate("Operating Mode"))
        htopmode:value("0",translate("Mixed Mode"))
        htopmode:value("1",translate("Green Mode"))
        htopmode.default = 0
	htgi = s:taboption("htmode", ListValue, "ht_gi", translate("Guard Interval"))
        htgi:value("0",translate("long"))
        htgi:value("1",translate("auto"))
        htgi.default = 1
	ht_rdg = s:taboption("htmode", ListValue, "ht_rdg", translate("Reverse Direction Grant(RDG)"))
        ht_rdg:value("0",translate("Disable"))
        ht_rdg:value("1",translate("Enable"))
        ht_rdg.default = 1
	ht_stbc = s:taboption("htmode", ListValue, "ht_stbc", translate("Space Time Block Coding (STBC)"))
        ht_stbc:value("0",translate("Disable"))
        ht_stbc:value("1",translate("Enable"))
        ht_stbc.default = 1
	ht_amsdu = s:taboption("htmode", ListValue, "ht_amsdu", translate("Aggregation MSDU(A-MSDU)"))
        ht_amsdu:value("0",translate("Disable"))
        ht_amsdu:value("1",translate("Enable"))
        ht_amsdu.default = 0
	ht_autoba = s:taboption("htmode", ListValue, "ht_autoba", translate("Auto Block ACK"))
        ht_autoba:value("0",translate("Disable"))
        ht_autoba:value("1",translate("Enable"))
        ht_autoba.default = 1
	ht_badec = s:taboption("htmode", ListValue, "ht_badec", translate("Decline BA Request"))
        ht_badec:value("0",translate("Disable"))
        ht_badec:value("1",translate("Enable"))
        ht_badec.default = 0
	ht_distkip = s:taboption("htmode", ListValue, "ht_distkip", translate("HT Disallow TKIP"))
        ht_distkip:value("0",translate("Disable"))
        ht_distkip:value("1",translate("Enable"))
        ht_distkip.default = 1
	ht_ldpc = s:taboption("htmode", ListValue, "ht_ldpc", translate("HT LDPC"))
        ht_ldpc:value("0",translate("Disable"))
        ht_ldpc:value("1",translate("Enable"))
        ht_ldpc.default = 0
--------------------------------------------VHT Physical Mode-------------------------------------------------
	vht_stbc = s:taboption("htmode", ListValue, "vht_stbc", translate("VHT STBC"))
        vht_stbc:depends("wifimode","2")
	vht_stbc:depends("wifimode","8")
	vht_stbc:depends("wifimode","14")
	vht_stbc:depends("wifimode","15")
	vht_stbc:value("0",translate("Disable"))
        vht_stbc:value("1",translate("Enable"))
        vht_stbc.default = 1
	vht_sgi = s:taboption("htmode", ListValue, "vht_sgi", translate("VHT Short GI"))
        vht_sgi:depends("wifimode","2")
	vht_sgi:depends("wifimode","8")
	vht_sgi:depends("wifimode","14")
	vht_sgi:depends("wifimode","15")
	vht_sgi:value("0",translate("Disable"))
        vht_sgi:value("1",translate("Enable"))
        vht_sgi.default = 0
	vht_bw_sig = s:taboption("htmode", ListValue, "vht_bw_sig", translate("VHT BW Signaling"))
        vht_bw_sig:depends("wifimode","2")
	vht_bw_sig:depends("wifimode","8")
	vht_bw_sig:depends("wifimode","14")
	vht_bw_sig:depends("wifimode","15")
	vht_bw_sig:value("0",translate("Disable"))
        vht_bw_sig:value("1",translate("Enable"))
        vht_bw_sig.default = 0
	vht_ldpc = s:taboption("htmode", ListValue, "vht_ldpc", translate("VHT LDPC"))
        vht_ldpc:depends("wifimode","2")
	vht_ldpc:depends("wifimode","8")
	vht_ldpc:depends("wifimode","14")
	vht_ldpc:depends("wifimode","15")
	vht_ldpc:value("0",translate("Disable"))
        vht_ldpc:value("1",translate("Enable"))
        vht_ldpc.default = 0
	ht_txstream = s:taboption("htmode", ListValue, "ht_txstream", translate("HT TxStream"))
	ht_txstream:value("1",translate("1"))
        ht_txstream:value("2",translate("2"))
        ht_txstream.default = 2
	ht_rxstream = s:taboption("htmode", ListValue, "ht_rxstream", translate("HT RxStream"))
        ht_rxstream:value("1",translate("1"))
        ht_rxstream:value("2",translate("2"))
        ht_rxstream.default = 2

	if htcaps then
		if hw_modes.g and hw_modes.n then mode:value("11ng", "802.11g+n") end
		if hw_modes.a and hw_modes.n then mode:value("11na", "802.11a+n") end

		htmode = s:taboption("advanced", ListValue, "htmode", translate("HT mode"))
		htmode:depends("hwmode", "11na")
		htmode:depends("hwmode", "11ng")
		htmode:value("HT20", "20MHz")
		htmode:value("HT40-", translate("40MHz 2nd channel below"))
		htmode:value("HT40+", translate("40MHz 2nd channel above"))

		noscan = s:taboption("advanced", Flag, "noscan", translate("Force 40MHz mode"),
			translate("Always use 40MHz channels even if the secondary channel overlaps. Using this option does not comply with IEEE 802.11n-2009!"))
		noscan:depends("htmode", "HT40+")
		noscan:depends("htmode", "HT40-")
		noscan.default = noscan.disabled

		--htcapab = s:taboption("advanced", DynamicList, "ht_capab", translate("HT capabilities"))
		--htcapab:depends("hwmode", "11na")
		--htcapab:depends("hwmode", "11ng")
	end

--	local cl = iw and iw.countrylist
--	if cl and #cl > 0 then
--		cc = s:taboption("advanced", ListValue, "country", translate("Country Code"), translate("Use ISO/IEC 3166 alpha2 country codes."))
--		cc.default = tostring(iw and iw.country or "00")
--		for _, c in ipairs(cl) do
--			cc:value(c.alpha2, "%s - %s" %{ c.alpha2, c.name })
--		end
--	else
--		s:taboption("advanced", Value, "country", translate("Country Code"), translate("Use ISO/IEC 3166 alpha2 country codes."))
--	end

--	s:taboption("advanced", Value, "distance", translate("Distance Optimization"),
--		translate("Distance to farthest network member in meters."))

-- external antenna profiles
--	local eal = iw and iw.extant
--	if eal and #eal > 0 then
--		ea = s:taboption("advanced", ListValue, "extant", translate("Antenna Configuration"))
--		for _, eap in ipairs(eal) do
--			ea:value(eap.id, "%s (%s)" %{ eap.name, eap.description })
--			if eap.selected then
--				ea.default = eap.id
--			end
--		end
--	end

--	s:taboption("advanced", Value, "frag", translate("Fragmentation Threshold"))
--	s:taboption("advanced", Value, "rts", translate("RTS/CTS Threshold"))
end


------------------- MAC80211 Device ------------------

if hwtype == "mac80211" then
	if #tx_power_list > 1 then
		tp = s:taboption("general", ListValue,
			"txpower", translate("Transmit Power"), "dBm")
		tp.rmempty = true
		tp.default = tx_power_cur
		function tp.cfgvalue(...)
			return txpower_current(Value.cfgvalue(...), tx_power_list)
		end

		for _, p in ipairs(tx_power_list) do
			tp:value(p.driver_dbm, "%i dBm (%i mW)"
				%{ p.display_dbm, p.display_mw })
		end
	end

	mode = s:taboption("advanced", ListValue, "hwmode", translate("Band"))

	if hw_modes.n then
		if hw_modes.g then mode:value("11g", "2.4GHz (802.11g+n)") end
		if hw_modes.a then mode:value("11a", "5GHz (802.11a+n)") end

		htmode = s:taboption("advanced", ListValue, "htmode", translate("HT mode (802.11n)"))
		htmode:value("", translate("disabled"))
		htmode:value("HT20", "20MHz")
		htmode:value("HT40", "40MHz")

		function mode.cfgvalue(...)
			local v = Value.cfgvalue(...)
			if v == "11na" then
				return "11a"
			elseif v == "11ng" then
				return "11g"
			end
			return v
		end

		noscan = s:taboption("advanced", Flag, "noscan", translate("Force 40MHz mode"),
			translate("Always use 40MHz channels even if the secondary channel overlaps. Using this option does not comply with IEEE 802.11n-2009!"))
		noscan:depends("htmode", "HT40")
		noscan.default = noscan.disabled
	else
		if hw_modes.g then mode:value("11g", "2.4GHz (802.11g)") end
		if hw_modes.a then mode:value("11a", "5GHz (802.11a)") end
	end

	local cl = iw and iw.countrylist
	if cl and #cl > 0 then
		cc = s:taboption("advanced", ListValue, "country", translate("Country Code"), translate("Use ISO/IEC 3166 alpha2 country codes."))
		cc.default = tostring(iw and iw.country or "00")
		for _, c in ipairs(cl) do
			cc:value(c.alpha2, "%s - %s" %{ c.alpha2, c.name })
		end
	else
		s:taboption("advanced", Value, "country", translate("Country Code"), translate("Use ISO/IEC 3166 alpha2 country codes."))
	end

	s:taboption("advanced", Value, "distance", translate("Distance Optimization"),
		translate("Distance to farthest network member in meters."))

	-- external antenna profiles
	local eal = iw and iw.extant
	if eal and #eal > 0 then
		ea = s:taboption("advanced", ListValue, "extant", translate("Antenna Configuration"))
		for _, eap in ipairs(eal) do
			ea:value(eap.id, "%s (%s)" %{ eap.name, eap.description })
			if eap.selected then
				ea.default = eap.id
			end
		end
	end

	s:taboption("advanced", Value, "frag", translate("Fragmentation Threshold"))
	s:taboption("advanced", Value, "rts", translate("RTS/CTS Threshold"))
end


------------------- Madwifi Device ------------------

if hwtype == "atheros" then
	tp = s:taboption("general",
		(#tx_power_list > 0) and ListValue or Value,
		"txpower", translate("Transmit Power"), "dBm")

	tp.rmempty = true
	tp.default = tx_power_cur

	function tp.cfgvalue(...)
		return txpower_current(Value.cfgvalue(...), tx_power_list)
	end

	for _, p in ipairs(tx_power_list) do
		tp:value(p.driver_dbm, "%i dBm (%i mW)"
			%{ p.display_dbm, p.display_mw })
	end

	mode = s:taboption("advanced", ListValue, "hwmode", translate("Mode"))
	mode:value("", translate("auto"))
	if hw_modes.b then mode:value("11b", "802.11b") end
	if hw_modes.g then mode:value("11g", "802.11g") end
	if hw_modes.a then mode:value("11a", "802.11a") end
	if hw_modes.g then mode:value("11bg", "802.11b+g") end
	if hw_modes.g then mode:value("11gst", "802.11g + Turbo") end
	if hw_modes.a then mode:value("11ast", "802.11a + Turbo") end
	mode:value("fh", translate("Frequency Hopping"))

	s:taboption("advanced", Flag, "diversity", translate("Diversity")).rmempty = false

	if not nsantenna then
		ant1 = s:taboption("advanced", ListValue, "txantenna", translate("Transmitter Antenna"))
		ant1.widget = "radio"
		ant1.orientation = "horizontal"
		ant1:depends("diversity", "")
		ant1:value("0", translate("auto"))
		ant1:value("1", translate("Antenna 1"))
		ant1:value("2", translate("Antenna 2"))

		ant2 = s:taboption("advanced", ListValue, "rxantenna", translate("Receiver Antenna"))
		ant2.widget = "radio"
		ant2.orientation = "horizontal"
		ant2:depends("diversity", "")
		ant2:value("0", translate("auto"))
		ant2:value("1", translate("Antenna 1"))
		ant2:value("2", translate("Antenna 2"))

	else -- NanoFoo
		local ant = s:taboption("advanced", ListValue, "antenna", translate("Transmitter Antenna"))
		ant:value("auto")
		ant:value("vertical")
		ant:value("horizontal")
		ant:value("external")
	end

	s:taboption("advanced", Value, "distance", translate("Distance Optimization"),
		translate("Distance to farthest network member in meters."))
	s:taboption("advanced", Value, "regdomain", translate("Regulatory Domain"))
	s:taboption("advanced", Value, "country", translate("Country Code"))
	s:taboption("advanced", Flag, "outdoor", translate("Outdoor Channels"))

	--s:option(Flag, "nosbeacon", translate("Disable HW-Beacon timer"))
end



------------------- Broadcom Device ------------------

if hwtype == "broadcom" then
	tp = s:taboption("general",
		(#tx_power_list > 0) and ListValue or Value,
		"txpower", translate("Transmit Power"), "dBm")

	tp.rmempty = true
	tp.default = tx_power_cur

	function tp.cfgvalue(...)
		return txpower_current(Value.cfgvalue(...), tx_power_list)
	end

	for _, p in ipairs(tx_power_list) do
		tp:value(p.driver_dbm, "%i dBm (%i mW)"
			%{ p.display_dbm, p.display_mw })
	end

	mode = s:taboption("advanced", ListValue, "hwmode", translate("Mode"))
	mode:value("11bg", "802.11b+g")
	mode:value("11b", "802.11b")
	mode:value("11g", "802.11g")
	mode:value("11gst", "802.11g + Turbo")

	ant1 = s:taboption("advanced", ListValue, "txantenna", translate("Transmitter Antenna"))
	ant1.widget = "radio"
	ant1:depends("diversity", "")
	ant1:value("3", translate("auto"))
	ant1:value("0", translate("Antenna 1"))
	ant1:value("1", translate("Antenna 2"))

	ant2 = s:taboption("advanced", ListValue, "rxantenna", translate("Receiver Antenna"))
	ant2.widget = "radio"
	ant2:depends("diversity", "")
	ant2:value("3", translate("auto"))
	ant2:value("0", translate("Antenna 1"))
	ant2:value("1", translate("Antenna 2"))

	s:taboption("advanced", Flag, "frameburst", translate("Frame Bursting"))

	s:taboption("advanced", Value, "distance", translate("Distance Optimization"))
	--s:option(Value, "slottime", translate("Slot time"))

	s:taboption("advanced", Value, "country", translate("Country Code"))
	s:taboption("advanced", Value, "maxassoc", translate("Connection Limit"))
end


--------------------- HostAP Device ---------------------

if hwtype == "prism2" then
	s:taboption("advanced", Value, "txpower", translate("Transmit Power"), "att units").rmempty = true

	s:taboption("advanced", Flag, "diversity", translate("Diversity")).rmempty = false

	s:taboption("advanced", Value, "txantenna", translate("Transmitter Antenna"))
	s:taboption("advanced", Value, "rxantenna", translate("Receiver Antenna"))
end


----------------------- Interface -----------------------

s = m:section(NamedSection, wnet.sid, "wifi-iface", translate("Interface Configuration"))
ifsection = s
s.addremove = false
s.anonymous = true
s.defaults.device = wdev:name()

s:tab("general", translate("General Setup"))
s:tab("encryption", translate("Wireless Security"))
s:tab("macfilter", translate("MAC-Filter"))
s:tab("advanced", translate("Advanced Settings"))

xessid=s:taboption("general", Value, "ssid", translate("<abbr title=\"Extended Service Set Identifier\">ESSID</abbr>"))
if vendor == "ralink" then
	xessid.datatype="maxlength(32)"
end

mode = s:taboption("general", ListValue, "mode", translate("Mode"))
mode.override_values = true
mode:value("ap", translate("Access Point"))
if vendor ~= "ralink" then
	mode:value("sta", translate("Client"))
	mode:value("adhoc", translate("Ad-Hoc"))
end

bssid = s:taboption("general", Value, "bssid", translate("<abbr title=\"Basic Service Set Identifier\">BSSID</abbr>"))

network = s:taboption("general", Value, "network", translate("Network"),
	translate("Choose the network(s) you want to attach to this wireless interface or " ..
		"fill out the <em>create</em> field to define a new network."))

network.rmempty = true
network.template = "cbi/network_netlist"
network.widget = "checkbox"
network.novirtual = true

function network.write(self, section, value)
	local i = nw:get_interface(section)
	if i then
		if value == '-' then
			value = m:formvalue(self:cbid(section) .. ".newnet")
			if value and #value > 0 then
				local n = nw:add_network(value, {proto="none"})
				if n then n:add_interface(i) end
			else
				local n = i:get_network()
				if n then n:del_interface(i) end
			end
		else
			local v
			for _, v in ipairs(i:get_networks()) do
				v:del_interface(i)
			end
			for v in ut.imatch(value) do
				local n = nw:get_network(v)
				if n then
					if not n:is_empty() then
						n:set("type", "bridge")
					end
					n:add_interface(i)
				end
			end
		end
	end
end


------------------ MTK Interface ----------------------

if vendor == "ralink" then
--	if fs.access("/usr/sbin/iw") then
--		mode:value("mesh", "802.11s")
--	end

--	mode:value("ahdemo", translate("Pseudo Ad-Hoc (ahdemo)"))
--	mode:value("monitor", translate("Monitor"))
	bssid:depends({mode="adhoc"})
	bssid:depends({mode="sta"})
	bssid:depends({mode="sta-wds"})

--	mp = s:taboption("macfilter", ListValue, "macfilter", translate("MAC-Address Filter"))
--	mp:depends({mode="ap"})
--	mp:depends({mode="ap-wds"})
--	mp:value("", translate("disable"))
--	mp:value("allow", translate("Allow listed only"))
--	mp:value("deny", translate("Allow all except listed"))

--	ml = s:taboption("macfilter", DynamicList, "maclist", translate("MAC-List"))
--	ml.datatype = "macaddr"
--	ml:depends({macfilter="allow"})
--	ml:depends({macfilter="deny"})
--	nt.mac_hints(function(mac, name) ml:value(mac, "%s (%s)" %{ mac, name }) end)

--	mode:value("ap-wds", "%s (%s)" % {translate("Access Point"), translate("WDS")})
--	mode:value("sta-wds", "%s (%s)" % {translate("Client"), translate("WDS")})

	function mode.write(self, section, value)
		if value == "ap-wds" then
			ListValue.write(self, section, "ap")
			m.uci:set("wireless", section, "wds", 1)
		elseif value == "sta-wds" then
			ListValue.write(self, section, "sta")
			m.uci:set("wireless", section, "wds", 1)
		else
			ListValue.write(self, section, value)
			m.uci:delete("wireless", section, "wds")
		end
	end

	function mode.cfgvalue(self, section)
		local mode = ListValue.cfgvalue(self, section)
		local wds  = m.uci:get("wireless", section, "wds") == "1"

		if mode == "ap" and wds then
			return "ap-wds"
		elseif mode == "sta" and wds then
			return "sta-wds"
		else
			return mode
		end
	end

--	hidden = s:taboption("general", Flag, "hidden", translate("Hide <abbr title=\"Extended Service Set Identifier\">ESSID</abbr>"))
--	hidden:depends({mode="ap"})
--	hidden:depends({mode="ap-wds"})
	wmm = s:taboption("general", ListValue, "wmm", translate("WMM Mode"))
        wmm:value("0","Disable")
        wmm:value("1","Enable")
        wmm.default = 1
        APSD = s:taboption("general", ListValue, "apsd", translate("APSDCapable"))
        APSD:value("0","Disable")
        APSD:value("1","Enable")
        APSD.default = 0
end

-------------------- MAC80211 Interface ----------------------

if hwtype == "mac80211" then
	if fs.access("/usr/sbin/iw") then
		mode:value("mesh", "802.11s")
	end

	mode:value("ahdemo", translate("Pseudo Ad-Hoc (ahdemo)"))
	mode:value("monitor", translate("Monitor"))
	bssid:depends({mode="adhoc"})
	bssid:depends({mode="sta"})
	bssid:depends({mode="sta-wds"})

	mp = s:taboption("macfilter", ListValue, "macfilter", translate("MAC-Address Filter"))
	mp:depends({mode="ap"})
	mp:depends({mode="ap-wds"})
	mp:value("", translate("disable"))
	mp:value("allow", translate("Allow listed only"))
	mp:value("deny", translate("Allow all except listed"))

	ml = s:taboption("macfilter", DynamicList, "maclist", translate("MAC-List"))
	ml.datatype = "macaddr"
	ml:depends({macfilter="allow"})
	ml:depends({macfilter="deny"})
	nt.mac_hints(function(mac, name) ml:value(mac, "%s (%s)" %{ mac, name }) end)

	mode:value("ap-wds", "%s (%s)" % {translate("Access Point"), translate("WDS")})
	mode:value("sta-wds", "%s (%s)" % {translate("Client"), translate("WDS")})

	function mode.write(self, section, value)
		if value == "ap-wds" then
			ListValue.write(self, section, "ap")
			m.uci:set("wireless", section, "wds", 1)
		elseif value == "sta-wds" then
			ListValue.write(self, section, "sta")
			m.uci:set("wireless", section, "wds", 1)
		else
			ListValue.write(self, section, value)
			m.uci:delete("wireless", section, "wds")
		end
	end

	function mode.cfgvalue(self, section)
		local mode = ListValue.cfgvalue(self, section)
		local wds  = m.uci:get("wireless", section, "wds") == "1"

		if mode == "ap" and wds then
			return "ap-wds"
		elseif mode == "sta" and wds then
			return "sta-wds"
		else
			return mode
		end
	end

	hidden = s:taboption("general", Flag, "hidden", translate("Hide <abbr title=\"Extended Service Set Identifier\">ESSID</abbr>"))
	hidden:depends({mode="ap"})
	hidden:depends({mode="ap-wds"})

	wmm = s:taboption("general", Flag, "wmm", translate("WMM Mode"))
	wmm:depends({mode="ap"})
	wmm:depends({mode="ap-wds"})
	wmm.default = wmm.enabled
end



-------------------- Madwifi Interface ----------------------

if hwtype == "atheros" then
	mode:value("ahdemo", translate("Pseudo Ad-Hoc (ahdemo)"))
	mode:value("monitor", translate("Monitor"))
	mode:value("ap-wds", "%s (%s)" % {translate("Access Point"), translate("WDS")})
	mode:value("sta-wds", "%s (%s)" % {translate("Client"), translate("WDS")})
	mode:value("wds", translate("Static WDS"))

	function mode.write(self, section, value)
		if value == "ap-wds" then
			ListValue.write(self, section, "ap")
			m.uci:set("wireless", section, "wds", 1)
		elseif value == "sta-wds" then
			ListValue.write(self, section, "sta")
			m.uci:set("wireless", section, "wds", 1)
		else
			ListValue.write(self, section, value)
			m.uci:delete("wireless", section, "wds")
		end
	end

	function mode.cfgvalue(self, section)
		local mode = ListValue.cfgvalue(self, section)
		local wds  = m.uci:get("wireless", section, "wds") == "1"

		if mode == "ap" and wds then
			return "ap-wds"
		elseif mode == "sta" and wds then
			return "sta-wds"
		else
			return mode
		end
	end

	bssid:depends({mode="adhoc"})
	bssid:depends({mode="ahdemo"})
	bssid:depends({mode="wds"})

	wdssep = s:taboption("advanced", Flag, "wdssep", translate("Separate WDS"))
	wdssep:depends({mode="ap-wds"})

	s:taboption("advanced", Flag, "doth", "802.11h")
	hidden = s:taboption("general", Flag, "hidden", translate("Hide <abbr title=\"Extended Service Set Identifier\">ESSID</abbr>"))
	hidden:depends({mode="ap"})
	hidden:depends({mode="adhoc"})
	hidden:depends({mode="ap-wds"})
	hidden:depends({mode="sta-wds"})
	isolate = s:taboption("advanced", Flag, "isolate", translate("Separate Clients"),
	 translate("Prevents client-to-client communication"))
	isolate:depends({mode="ap"})
	s:taboption("advanced", Flag, "bgscan", translate("Background Scan"))

	mp = s:taboption("macfilter", ListValue, "macpolicy", translate("MAC-Address Filter"))
	mp:value("", translate("disable"))
	mp:value("allow", translate("Allow listed only"))
	mp:value("deny", translate("Allow all except listed"))

	ml = s:taboption("macfilter", DynamicList, "maclist", translate("MAC-List"))
	ml.datatype = "macaddr"
	ml:depends({macpolicy="allow"})
	ml:depends({macpolicy="deny"})
	nt.mac_hints(function(mac, name) ml:value(mac, "%s (%s)" %{ mac, name }) end)

	s:taboption("advanced", Value, "rate", translate("Transmission Rate"))
	s:taboption("advanced", Value, "mcast_rate", translate("Multicast Rate"))
	s:taboption("advanced", Value, "frag", translate("Fragmentation Threshold"))
	s:taboption("advanced", Value, "rts", translate("RTS/CTS Threshold"))
	s:taboption("advanced", Value, "minrate", translate("Minimum Rate"))
	s:taboption("advanced", Value, "maxrate", translate("Maximum Rate"))
	s:taboption("advanced", Flag, "compression", translate("Compression"))

	s:taboption("advanced", Flag, "bursting", translate("Frame Bursting"))
	s:taboption("advanced", Flag, "turbo", translate("Turbo Mode"))
	s:taboption("advanced", Flag, "ff", translate("Fast Frames"))

	s:taboption("advanced", Flag, "wmm", translate("WMM Mode"))
	s:taboption("advanced", Flag, "xr", translate("XR Support"))
	s:taboption("advanced", Flag, "ar", translate("AR Support"))

	local swm = s:taboption("advanced", Flag, "sw_merge", translate("Disable HW-Beacon timer"))
	swm:depends({mode="adhoc"})

	local nos = s:taboption("advanced", Flag, "nosbeacon", translate("Disable HW-Beacon timer"))
	nos:depends({mode="sta"})
	nos:depends({mode="sta-wds"})

	local probereq = s:taboption("advanced", Flag, "probereq", translate("Do not send probe responses"))
	probereq.enabled  = "0"
	probereq.disabled = "1"
end


-------------------- Broadcom Interface ----------------------

if hwtype == "broadcom" then
	mode:value("wds", translate("WDS"))
	mode:value("monitor", translate("Monitor"))

	hidden = s:taboption("general", Flag, "hidden", translate("Hide <abbr title=\"Extended Service Set Identifier\">ESSID</abbr>"))
	hidden:depends({mode="ap"})
	hidden:depends({mode="adhoc"})
	hidden:depends({mode="wds"})

	isolate = s:taboption("advanced", Flag, "isolate", translate("Separate Clients"),
	 translate("Prevents client-to-client communication"))
	isolate:depends({mode="ap"})

	s:taboption("advanced", Flag, "doth", "802.11h")
	s:taboption("advanced", Flag, "wmm", translate("WMM Mode"))

	bssid:depends({mode="wds"})
	bssid:depends({mode="adhoc"})
end


----------------------- HostAP Interface ---------------------

if hwtype == "prism2" then
	mode:value("wds", translate("WDS"))
	mode:value("monitor", translate("Monitor"))

	hidden = s:taboption("general", Flag, "hidden", translate("Hide <abbr title=\"Extended Service Set Identifier\">ESSID</abbr>"))
	hidden:depends({mode="ap"})
	hidden:depends({mode="adhoc"})
	hidden:depends({mode="wds"})

	bssid:depends({mode="sta"})

	mp = s:taboption("macfilter", ListValue, "macpolicy", translate("MAC-Address Filter"))
	mp:value("", translate("disable"))
	mp:value("allow", translate("Allow listed only"))
	mp:value("deny", translate("Allow all except listed"))
	ml = s:taboption("macfilter", DynamicList, "maclist", translate("MAC-List"))
	ml:depends({macpolicy="allow"})
	ml:depends({macpolicy="deny"})
	nt.mac_hints(function(mac, name) ml:value(mac, "%s (%s)" %{ mac, name }) end)

	s:taboption("advanced", Value, "rate", translate("Transmission Rate"))
	s:taboption("advanced", Value, "frag", translate("Fragmentation Threshold"))
	s:taboption("advanced", Value, "rts", translate("RTS/CTS Threshold"))
end


------------------- WiFI-Encryption -------------------

encr = s:taboption("encryption", ListValue, "encryption", translate("Encryption"))
encr.override_values = true
encr.override_depends = true
encr:depends({mode="ap"})
encr:depends({mode="sta"})
encr:depends({mode="adhoc"})
encr:depends({mode="ahdemo"})
encr:depends({mode="ap-wds"})
encr:depends({mode="sta-wds"})
encr:depends({mode="mesh"})

cipher = s:taboption("encryption", ListValue, "cipher", translate("Cipher"))
cipher:depends({encryption="wpa"})
cipher:depends({encryption="wpa2"})
cipher:depends({encryption="psk"})
cipher:depends({encryption="psk2"})
cipher:depends({encryption="wpa-mixed"})
cipher:depends({encryption="psk-mixed"})
if vendor ~= "ralink" then
	cipher:value("auto", translate("auto"))
end
cipher:value("ccmp", translate("Force CCMP (AES)"))
cipher:value("tkip", translate("Force TKIP"))
cipher:value("tkip+ccmp", translate("Force TKIP and CCMP (AES)"))

function encr.cfgvalue(self, section)
	local v = tostring(ListValue.cfgvalue(self, section))
	if v == "wep" then
		return "wep-open"
	elseif v and v:match("%+") then
		return (v:gsub("%+.+$", ""))
	end
	return v
end

function encr.write(self, section, value)
	local e = tostring(encr:formvalue(section))
	local c = tostring(cipher:formvalue(section))
	if value == "wpa" or value == "wpa2"  then
		self.map.uci:delete("wireless", section, "key")
	end
	if e and (c == "tkip" or c == "ccmp" or c == "tkip+ccmp") then
		e = e .. "+" .. c
	end
	self.map:set(section, "encryption", e)
end

function cipher.cfgvalue(self, section)
	local v = tostring(ListValue.cfgvalue(encr, section))
	if v and v:match("%+") then
		v = v:gsub("^[^%+]+%+", "")
		if v == "aes" then v = "ccmp"
		elseif v == "tkip+aes" then v = "tkip+ccmp"
		elseif v == "aes+tkip" then v = "tkip+ccmp"
		elseif v == "ccmp+tkip" then v = "tkip+ccmp"
		end
	end
	return v
end

function cipher.write(self, section)
	return encr:write(section)
end


encr:value("none", "No Encryption")
encr:value("wep-open",   translate("WEP Open System"), {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"}, {mode="adhoc"}, {mode="ahdemo"}, {mode="wds"})
encr:value("wep-shared", translate("WEP Shared Key"),  {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"}, {mode="adhoc"}, {mode="ahdemo"}, {mode="wds"})

if hwtype == "atheros" or vendor == "ralink" or hwtype == "mac80211" or hwtype == "prism2" then
	local supplicant = fs.access("/usr/sbin/wpa_supplicant")
	local hostapd = fs.access("/usr/sbin/hostapd")

	-- Probe EAP support
	local has_ap_eap  = (os.execute("hostapd -veap >/dev/null 2>/dev/null") == 0)
	local has_sta_eap = (os.execute("wpa_supplicant -veap >/dev/null 2>/dev/null") == 0)
	if vendor == "ralink" then
		-- for mtk
		encr:value("psk", "WPA-PSK", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		encr:value("psk2", "WPA2-PSK", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		encr:value("psk-mixed", "WPA-PSK/WPA2-PSK Mixed Mode", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		-- if has_ap_eap and has_sta_eap then
			encr:value("wpa", "WPA-EAP", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
			encr:value("wpa2", "WPA2-EAP", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		-- end
			encr:value("wpa-mixed", "WPA-EAP/WPA2-EAP Mixed Mode", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
			encr:value("8021x", "8021x", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		--------------------------------------------------------------------------------------------------------
	end
	if hostapd and supplicant then
		encr:value("psk", "WPA-PSK", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		encr:value("psk2", "WPA2-PSK", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		encr:value("psk-mixed", "WPA-PSK/WPA2-PSK Mixed Mode", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		if vendor == "ralink" then
			encr:value("wpa-mixed", "WPA-EAP/WPA2-EAP Mixed Mode", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
			encr:value("8021x", "8021x", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		elseif has_ap_eap and has_sta_eap then
			encr:value("wpa", "WPA-EAP", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
			encr:value("wpa2", "WPA2-EAP", {mode="ap"}, {mode="sta"}, {mode="ap-wds"}, {mode="sta-wds"})
		end
	elseif hostapd and not supplicant then
		encr:value("psk", "WPA-PSK", {mode="ap"}, {mode="ap-wds"})
		encr:value("psk2", "WPA2-PSK", {mode="ap"}, {mode="ap-wds"})
		encr:value("psk-mixed", "WPA-PSK/WPA2-PSK Mixed Mode", {mode="ap"}, {mode="ap-wds"})
		if has_ap_eap then
			encr:value("wpa", "WPA-EAP", {mode="ap"}, {mode="ap-wds"})
			encr:value("wpa2", "WPA2-EAP", {mode="ap"}, {mode="ap-wds"})
		end
		encr.description = translate(
			"WPA-Encryption requires wpa_supplicant (for client mode) or hostapd (for AP " ..
			"and ad-hoc mode) to be installed."
		)
	elseif not hostapd and supplicant then
		encr:value("psk", "WPA-PSK", {mode="sta"}, {mode="sta-wds"})
		encr:value("psk2", "WPA2-PSK", {mode="sta"}, {mode="sta-wds"})
		encr:value("psk-mixed", "WPA-PSK/WPA2-PSK Mixed Mode", {mode="sta"}, {mode="sta-wds"})
		if has_sta_eap then
			encr:value("wpa", "WPA-EAP", {mode="sta"}, {mode="sta-wds"})
			encr:value("wpa2", "WPA2-EAP", {mode="sta"}, {mode="sta-wds"})
		end
		if vendor ~= "ralink" then
			encr.description = translate(
				"WPA-Encryption requires wpa_supplicant (for client mode) or hostapd (for AP " ..
				"and ad-hoc mode) to be installed."
			)
		end
	else
		encr.description = translate(
			"WPA-Encryption requires wpa_supplicant (for client mode) or hostapd (for AP " ..
			"and ad-hoc mode) to be installed."
		)
	end
elseif hwtype == "broadcom" then
	encr:value("psk", "WPA-PSK")
	encr:value("psk2", "WPA2-PSK")
	encr:value("psk+psk2", "WPA-PSK/WPA2-PSK Mixed Mode")
end

if vendor == "ralink" then
	rekeyinteval= s:taboption("encryption", Value, "rekeyinteval", translate("Key Renewal Interval(seconds)"))
	rekeyinteval:depends({mode="ap", encryption="wpa"})
	rekeyinteval:depends({mode="ap", encryption="wpa2"})
	rekeyinteval:depends({mode="ap", encryption="wpa-mixed"})
	rekeyinteval:depends({mode="ap", encryption="psk"})
	rekeyinteval:depends({mode="ap", encryption="psk2"})
	rekeyinteval:depends({mode="ap", encryption="psk-mixed"})
	rekeyinteval:depends({mode="ap-wds", encryption="wpa"})
	rekeyinteval:depends({mode="ap-wds", encryption="wpa2"})
	rekeyinteval:depends({mode="ap-wds", encryption="wpa-mixed"})
	rekeyinteval:depends({mode="ap-wds", encryption="psk"})
	rekeyinteval:depends({mode="ap-wds", encryption="psk2"})
	rekeyinteval:depends({mode="ap-wds", encryption="psk-mixed"})
	rekeyinteval.datatype = "range(0,4194303)"

	pmkcacheperiod = s:taboption("encryption", Value, "pmkcacheperiod", translate("PMK Cache Period(minutes)"))
	pmkcacheperiod:depends({mode="ap", encryption="wpa2"})
	pmkcacheperiod:depends({mode="ap-wds", encryption="wpa2"})
	pmkcacheperiod.datatype = "range(0,65535)"

	preauth = s:taboption("encryption", ListValue, "preauth", translate("Pre-Authentication"))
	preauth:depends({mode="ap", encryption="wpa2"})
	preauth:depends({mode="ap-wds", encryption="wpa2"})
	preauth:value("0",translate("Disable"))
	preauth:value("1",translate("Enable"))
	preauth.default = "0"
end

auth_server = s:taboption("encryption", Value, "auth_server", translate("Radius-Authentication-Server"))
auth_server:depends({mode="ap", encryption="wpa"})
auth_server:depends({mode="ap", encryption="wpa2"})
if vendor == "ralink" then
	auth_server:depends({mode="ap", encryption="wpa-mixed"})
	auth_server:depends({mode="ap", encryption="8021x"})
end
auth_server:depends({mode="ap-wds", encryption="wpa"})
auth_server:depends({mode="ap-wds", encryption="wpa2"})
if vendor == "ralink" then
	auth_server:depends({mode="ap-wds", encryption="wpa-mixed"})
	auth_server:depends({mode="ap-wds", encryption="8021x"})
end
auth_server.rmempty = true
auth_server.datatype = "host"

auth_port = s:taboption("encryption", Value, "auth_port", translate("Radius-Authentication-Port"), translatef("Default %d", 1812))
auth_port:depends({mode="ap", encryption="wpa"})
auth_port:depends({mode="ap", encryption="wpa2"})
if vendor == "ralink" then
	auth_port:depends({mode="ap", encryption="wpa-mixed"})
	auth_port:depends({mode="ap", encryption="8021x"})
end
auth_port:depends({mode="ap-wds", encryption="wpa"})
auth_port:depends({mode="ap-wds", encryption="wpa2"})
if vendor == "ralink" then
	auth_port:depends({mode="ap-wds", encryption="wpa-mixed"})
	auth_port:depends({mode="ap-wds", encryption="8021x"})
end
auth_port.rmempty = true
auth_port.datatype = "port"

auth_secret = s:taboption("encryption", Value, "auth_secret", translate("Radius-Authentication-Secret"))
auth_secret:depends({mode="ap", encryption="wpa"})
auth_secret:depends({mode="ap", encryption="wpa2"})
if vendor == "ralink" then
	auth_secret:depends({mode="ap", encryption="wpa-mixed"})
	auth_secret:depends({mode="ap", encryption="8021x"})
end
auth_secret:depends({mode="ap-wds", encryption="wpa"})
auth_secret:depends({mode="ap-wds", encryption="wpa2"})
if vendor == "ralink" then
	auth_secret:depends({mode="ap-wds", encryption="wpa-mixed"})
	auth_secret:depends({mode="ap-wds", encryption="8021x"})
end
auth_secret.rmempty = true
auth_secret.password = true

acct_server = s:taboption("encryption", Value, "acct_server", translate("Radius-Accounting-Server"))
acct_server:depends({mode="ap", encryption="wpa"})
acct_server:depends({mode="ap", encryption="wpa2"})
if vendor == "ralink" then
	acct_server:depends({mode="ap", encryption="wpa-mixed"})
end
acct_server:depends({mode="ap-wds", encryption="wpa"})
acct_server:depends({mode="ap-wds", encryption="wpa2"})
if vendor == "ralink" then
	acct_server:depends({mode="ap-wds", encryption="wpa-mixed"})
end
acct_server.rmempty = true
acct_server.datatype = "host"

acct_port = s:taboption("encryption", Value, "acct_port", translate("Radius-Accounting-Port"), translatef("Default %d", 1813))
acct_port:depends({mode="ap", encryption="wpa"})
acct_port:depends({mode="ap", encryption="wpa2"})
if vendor == "ralink" then
	acct_port:depends({mode="ap", encryption="wpa-mixed"})
end
acct_port:depends({mode="ap-wds", encryption="wpa"})
acct_port:depends({mode="ap-wds", encryption="wpa2"})
if vendor == "ralink" then
	acct_port:depends({mode="ap-wds", encryption="wpa-mixed"})
end
acct_port.rmempty = true
acct_port.datatype = "port"

acct_secret = s:taboption("encryption", Value, "acct_secret", translate("Radius-Accounting-Secret"))
acct_secret:depends({mode="ap", encryption="wpa"})
acct_secret:depends({mode="ap", encryption="wpa2"})
if vendor == "ralink" then
	acct_secret:depends({mode="ap", encryption="wpa-mixed"})
end
acct_secret:depends({mode="ap-wds", encryption="wpa"})
acct_secret:depends({mode="ap-wds", encryption="wpa2"})
if vendor == "ralink" then
	acct_secret:depends({mode="ap-wds", encryption="wpa-mixed"})
end
acct_secret.rmempty = true
acct_secret.password = true

wpakey = s:taboption("encryption", Value, "_wpa_key", translate("Key"))
wpakey:depends("encryption", "psk")
wpakey:depends("encryption", "psk2")
wpakey:depends("encryption", "psk+psk2")
wpakey:depends("encryption", "psk-mixed")
wpakey.datatype = "wpakey"
wpakey.rmempty = true
wpakey.password = true

wpakey.cfgvalue = function(self, section, value)
	local key = m.uci:get("wireless", section, "key")
	if key == "1" or key == "2" or key == "3" or key == "4" then
		return nil
	end
	return key
end

wpakey.write = function(self, section, value)
	self.map.uci:set("wireless", section, "key", value)
	self.map.uci:delete("wireless", section, "key1")
end


wepslot = s:taboption("encryption", ListValue, "_wep_key", translate("Used Key Slot"))
wepslot:depends("encryption", "wep-open")
wepslot:depends("encryption", "wep-shared")
wepslot:value("1", translatef("Key #%d", 1))
wepslot:value("2", translatef("Key #%d", 2))
wepslot:value("3", translatef("Key #%d", 3))
wepslot:value("4", translatef("Key #%d", 4))

wepslot.cfgvalue = function(self, section)
	local slot = tonumber(m.uci:get("wireless", section, "key"))
	if not slot or slot < 1 or slot > 4 then
		return 1
	end
	return slot
end

wepslot.write = function(self, section, value)
	self.map.uci:set("wireless", section, "key", value)
end

local slot
for slot=1,4 do
	wepkey = s:taboption("encryption", Value, "key" .. slot, translatef("Key #%d", slot))
	wepkey:depends("encryption", "wep-open")
	wepkey:depends("encryption", "wep-shared")
	wepkey.datatype = "wepkey"
	wepkey.rmempty = true
	wepkey.password = true

	function wepkey.write(self, section, value)
		if value and (#value == 5 or #value == 13) then
			value = "s:" .. value
		end
		return Value.write(self, section, value)
	end
end


if hwtype == "atheros" or vendor == "ralink" or hwtype == "mac80211" or hwtype == "prism2" then
	nasid = s:taboption("encryption", Value, "nasid", translate("NAS ID"))
	nasid:depends({mode="ap", encryption="wpa"})
	nasid:depends({mode="ap", encryption="wpa2"})
	if vendor == "ralink" then
		nasid:depends({mode="ap", encryption="wpa-mixed"})
	end
	nasid:depends({mode="ap-wds", encryption="wpa"})
	nasid:depends({mode="ap-wds", encryption="wpa2"})
	if vendor == "ralink" then
		nasid:depends({mode="ap-wds", encryption="wpa-mixed"})
	end
	nasid.rmempty = true

	eaptype = s:taboption("encryption", ListValue, "eap_type", translate("EAP-Method"))
	eaptype:value("tls",  "TLS")
	eaptype:value("ttls", "TTLS")
	eaptype:value("peap", "PEAP")
	eaptype:depends({mode="sta", encryption="wpa"})
	eaptype:depends({mode="sta", encryption="wpa2"})
	if vendor == "ralink" then
		eaptype:depends({mode="sta", encryption="wpa-mixed"})
	end
	eaptype:depends({mode="sta-wds", encryption="wpa"})
	eaptype:depends({mode="sta-wds", encryption="wpa2"})
	if vendor == "ralink" then
		eaptype:depends({mode="sta-wds", encryption="wpa-mixed"})
	end

	cacert = s:taboption("encryption", FileUpload, "ca_cert", translate("Path to CA-Certificate"))
	cacert:depends({mode="sta", encryption="wpa"})
	cacert:depends({mode="sta", encryption="wpa2"})
	if vendor == "ralink" then
		cacert:depends({mode="sta", encryption="wpa-mixed"})
	end
	cacert:depends({mode="sta-wds", encryption="wpa"})
	cacert:depends({mode="sta-wds", encryption="wpa2"})
	if vendor == "ralink" then
		cacert:depends({mode="sta-wds", encryption="wpa-mixed"})
	end

	clientcert = s:taboption("encryption", FileUpload, "client_cert", translate("Path to Client-Certificate"))
	clientcert:depends({mode="sta", encryption="wpa"})
	clientcert:depends({mode="sta", encryption="wpa2"})
	if vendor == "ralink" then
		clientcert:depends({mode="sta", encryption="wpa-mixed"})
	end
	clientcert:depends({mode="sta-wds", encryption="wpa"})
	clientcert:depends({mode="sta-wds", encryption="wpa2"})
	if vendor == "ralink" then
		clientcert:depends({mode="sta-wds", encryption="wpa-mixed"})
	end

	privkey = s:taboption("encryption", FileUpload, "priv_key", translate("Path to Private Key"))
	privkey:depends({mode="sta", eap_type="tls", encryption="wpa2"})
	privkey:depends({mode="sta", eap_type="tls", encryption="wpa"})
	if vendor == "ralink" then
		privkey:depends({mode="sta", eap_type="tls", encryption="wpa-mixed"})
	end
	privkey:depends({mode="sta-wds", eap_type="tls", encryption="wpa2"})
	privkey:depends({mode="sta-wds", eap_type="tls", encryption="wpa"})
	if vendor == "ralink" then
		privkey:depends({mode="sta-wds", eap_type="tls", encryption="wpa-mixed"})
	end

	privkeypwd = s:taboption("encryption", Value, "priv_key_pwd", translate("Password of Private Key"))
	privkeypwd:depends({mode="sta", eap_type="tls", encryption="wpa2"})
	privkeypwd:depends({mode="sta", eap_type="tls", encryption="wpa"})
	if vendor == "ralink" then
		privkeypwd:depends({mode="sta", eap_type="tls", encryption="wpa-mixed"})
	end
	privkeypwd:depends({mode="sta-wds", eap_type="tls", encryption="wpa2"})
	privkeypwd:depends({mode="sta-wds", eap_type="tls", encryption="wpa"})
	if vendor == "ralink" then
		privkeypwd:depends({mode="sta-wds", eap_type="tls", encryption="wpa-mixed"})
	end


	auth = s:taboption("encryption", Value, "auth", translate("Authentication"))
	auth:value("PAP")
	auth:value("CHAP")
	auth:value("MSCHAP")
	auth:value("MSCHAPV2")
	auth:depends({mode="sta", eap_type="peap", encryption="wpa2"})
	auth:depends({mode="sta", eap_type="peap", encryption="wpa"})
	if vendor == "ralink" then
		auth:depends({mode="sta", eap_type="peap", encryption="wpa-mixed"})
	end
	auth:depends({mode="sta", eap_type="ttls", encryption="wpa2"})
	auth:depends({mode="sta", eap_type="ttls", encryption="wpa"})
	if vendor == "ralink" then
		auth:depends({mode="sta", eap_type="ttls", encryption="wpa-mixed"})
	end
	auth:depends({mode="sta-wds", eap_type="peap", encryption="wpa2"})
	auth:depends({mode="sta-wds", eap_type="peap", encryption="wpa"})
	if vendor == "ralink" then
		auth:depends({mode="sta-wds", eap_type="peap", encryption="wpa-mixed"})
	end
	auth:depends({mode="sta-wds", eap_type="ttls", encryption="wpa2"})
	auth:depends({mode="sta-wds", eap_type="ttls", encryption="wpa"})
	if vendor == "ralink" then
		auth:depends({mode="sta-wds", eap_type="ttls", encryption="wpa-mixed"})
	end


	identity = s:taboption("encryption", Value, "identity", translate("Identity"))
	identity:depends({mode="sta", eap_type="peap", encryption="wpa2"})
	identity:depends({mode="sta", eap_type="peap", encryption="wpa"})
	if vendor == "ralink" then
		identity:depends({mode="sta", eap_type="peap", encryption="wpa-mixed"})
	end
	identity:depends({mode="sta", eap_type="ttls", encryption="wpa2"})
	identity:depends({mode="sta", eap_type="ttls", encryption="wpa"})
	if vendor == "ralink" then
		identity:depends({mode="sta", eap_type="ttls", encryption="wpa-mixed"})
	end
	identity:depends({mode="sta-wds", eap_type="peap", encryption="wpa2"})
	identity:depends({mode="sta-wds", eap_type="peap", encryption="wpa"})
	if vendor == "ralink" then
		identity:depends({mode="sta-wds", eap_type="peap", encryption="wpa-mixed"})
	end
	identity:depends({mode="sta-wds", eap_type="ttls", encryption="wpa2"})
	identity:depends({mode="sta-wds", eap_type="ttls", encryption="wpa"})
	if vendor == "ralink" then
		identity:depends({mode="sta-wds", eap_type="ttls", encryption="wpa-mixed"})
	end

	password = s:taboption("encryption", Value, "password", translate("Password"))
	password:depends({mode="sta", eap_type="peap", encryption="wpa2"})
	password:depends({mode="sta", eap_type="peap", encryption="wpa"})
	if vendor == "ralink" then
		password:depends({mode="sta", eap_type="peap", encryption="wpa-mixed"})
	end
	password:depends({mode="sta", eap_type="ttls", encryption="wpa2"})
	password:depends({mode="sta", eap_type="ttls", encryption="wpa"})
	if vendor == "ralink" then
		password:depends({mode="sta", eap_type="ttls", encryption="wpa-mixed"})
	end
	password:depends({mode="sta-wds", eap_type="peap", encryption="wpa2"})
	password:depends({mode="sta-wds", eap_type="peap", encryption="wpa"})
	if vendor == "ralink" then
		password:depends({mode="sta-wds", eap_type="peap", encryption="wpa-mixed"})
	end
	password:depends({mode="sta-wds", eap_type="ttls", encryption="wpa2"})
	password:depends({mode="sta-wds", eap_type="ttls", encryption="wpa"})
	if vendor == "ralink" then
		password:depends({mode="sta-wds", eap_type="ttls", encryption="wpa-mixed"})
	end
end

return m
