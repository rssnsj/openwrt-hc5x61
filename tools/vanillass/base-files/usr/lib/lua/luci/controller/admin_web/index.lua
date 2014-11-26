--[[
	Info	界面控制层主入口
	Author	Wangchao  <wangchao123.com@gmail.com>
	Copyright	2012
]]--

module("luci.controller.admin_web.index", package.seeall)

-- 首次登登陆无需认证


function index()
	--作为入口
	local root = node()
	if not root.target then
		root.target = alias("admin_web")
		root.index = true
	end

	local page   = node("admin_web")
	page.target  = firstchild()
	page.title   = _("")
	page.order   = 10
	page.sysauth = "admin"
	page.mediaurlbase = "/turbo-static/turbo/web"
	local superkey = (luci.http.getcookie("superkey"))
  if superkey ~= nil then
     page.sysauth_authenticator = "appauth"
  else
     page.sysauth_authenticator = "htmlauth_web"
  end
	page.index = true
	
	local guide_access_tag
		
	local HAVEBEENSET_V = luci.util.get_agreement("HAVEBEENSET")
	if HAVEBEENSET_V == 0 or HAVEBEENSET_V == "0" then
		guide_access_tag = true
	else 
		guide_access_tag = false
	end
	
	--首页 & 系统
	entry({"admin_web"}, template("admin_web/home"), _("运行状态"), 10)
	entry({"admin_web", "home"}, template("admin_web/home"), _("运行状态"), 10)
	entry({"admin_web", "logout"}, call("action_logout"), 11, true)
	entry({"admin_web", "info"}, template("admin_web/sysinfo"), _("运行状态"), 13)
	
	--网络设置
	entry({"admin_web", "network"}, template("admin_web/network/net_setup"), _("网络设置"), 20)
	entry({"admin_web", "network","setup"}, template("admin_web/network/net_setup"), _("网络设置"), 20)
	entry({"admin_web", "network","setup","autowantype"}, template("admin_web/network/net_setup_autowantype"), _("网络设置"), 20)
	entry({"admin_web", "network","setup","lan"}, template("admin_web/network/net_setup_lan"), _("网络设置"), 30)
	entry({"admin_web", "network", "setup","lan_proc"}, call("lan_proc"), nil)
	entry({"admin_web", "network", "setup","dhcp"}, template("admin_web/network/net_setup_dhcp"), _("DHCP 服务器"), 40)
	entry({"admin_web", "network", "setup","mtu"}, template("admin_web/network/net_setup_mtu"), _("mtu 设置"), 40)
	entry({"admin_web", "network", "setup","mac"}, template("admin_web/network/net_setup_mac"), _("mac 设置"), 40)
	entry({"admin_web", "network", "setup","ppp_adv"}, template("admin_web/network/ppp_adv_setup"), "PPP 高级设置", 40)
	entry({"admin_web", "network","devices_list"}, template("admin_web/network/devices_list"), _("链接设备列表"), 200)
	entry({"admin_web", "network","upnp"}, template("admin_web/network/upnp"), _("upnp 状态"), 200)
	entry({"admin_web", "network","l2tp"}, template("admin_web/network/l2tp"), _("l2tp 状态"), 210)
	entry({"admin_web", "network","pppoe_log"}, template("admin_web/network/pppoe_log"), _("pppoe log"), 210)
	
	--个人中心
	entry({"admin_web", "passport"}, template("admin_web/passport/bind"), _("个人中心"), 30)
	entry({"admin_web", "passport", "bind"}, template("admin_web/passport/bind"), _("个人中心"), 30)
	entry({"admin_web", "passport", "bind", "callback"},  template("admin_web/passport/bind"), _("绑定路由器回调"), 97)
	entry({"admin_web", "passport", "apps"}, template("admin_web/passport/apps"), _("精彩应用"), 30)
	
	--系统管理
	entry({"admin_web", "system"}, firstchild(), _("系统管理"), 190)
	entry({"admin_web", "system"}, template("admin_web/system/setup"), _("系统管理"), 198)
	entry({"admin_web", "system","setup"}, template("admin_web/system/setup"), _("系统管理"), 198)
	entry({"admin_web", "system","systime"}, template("admin_web/system/systime"), _("系统时间"), 199)
	
	entry({"admin_web", "system","upgrade"}, template("admin_web/system/upgrade"), _("路由器固件升级"), 198)
	entry({"admin_web", "system","upgrade_user"}, template("admin_web/system/upgrade_user"), _("路由器固件升级"), 198)	--不需要登录
	entry({"admin_web", "system","backNrestore"}, template("admin_web/system/backNrestore"), _("备份/载入配置"), 198)
	entry({"admin_web", "system","reboot_reset"}, template("admin_web/system/reboot_reset"), _("重启/恢复出厂设置"), 198)
	entry({"admin_web", "system","agreement"}, template("admin_web/system/agreement"), _("用户协议"), 198,true)	--不需要登录
	entry({"admin_web", "system","net_detect"}, template("admin_web/system/net_detect"), _("网络监测"), 192,true).leaf = true
	entry({"admin_web", "system","net_detect_1"}, template("admin_web/system/net_detect_1"), _("网络监测1.1"), 199,true).leaf = true
	entry({"admin_web", "system","net_detect_2"}, template("admin_web/system/net_detect_2"), _("网络监测2"), 199,true).leaf = true
	entry({"admin_web", "system","net_detect_help_2"}, template("admin_web/system/net_detect_help_2"), _("网络诊断帮助"), 199,true).leaf = true
	entry({"admin_web", "system","disk"}, template("admin_web/system/disk"), _("存储诊断"), 192).leaf = true
	entry({"admin_web", "system","phone_app_open"}, template("admin_web/system/phone_app_open"), _("引导安装或打开 APP"), 199,true).leaf = true
	entry({"admin_web", "system","sd_manual_part"}, template("admin_web/system/sd_manual_part"), _("格式化SD卡"), 199).leaf = true
	entry({"admin_web", "system","sata_manual_part"}, template("admin_web/system/sata_manual_part"), _("格式化硬盘"), 199).leaf = true
	
	--WIFI
	entry({"admin_web", "wifi"}, firstchild(), _("无线设置"), 50)
	entry({"admin_web", "wifi", "setup"}, template("admin_web/wifi/wifi_setup"), _("无线设置"), 30)
	entry({"admin_web", "wifi", "setup","channel"}, template("admin_web/wifi/wifi_setup_channel"), _("无线频道/信号强度"), 60)
	entry({"admin_web", "wifi","devices_list"}, template("admin_web/wifi/wifi_devices_list"), _("wifi链接设备列表"), 60)
	entry({"admin_web", "wifi", "setup","mac_filter"}, template("admin_web/wifi/wifi_mac_filter"),  _("网络设置"),30)
	
	--plugin
	entry({"admin_web", "plugin"}, firstchild(), _("插件"), 50)
	entry({"admin_web", "plugin", "mentohust"}, template("admin_web/plugin/mentohust"), _("锐捷认证"), 30)
	entry({"admin_web", "plugin", "x3c"}, template("admin_web/plugin/x3c"), _("华3认证"), 30)
	entry({"admin_web", "plugin", "logs"}, template("admin_web/plugin/logs"), _("logs"), 30)
	entry({"admin_web", "plugin", "shadowsocks"}, template("admin_web/plugin/shadowsocks"), _("Shadowsocks"), 30)
	
end

--退出
function action_logout()
	local dsp = require "luci.dispatcher"
	local sauth = require "luci.sauth"
	if dsp.context.authsession then
		sauth.kill(dsp.context.authsession)
		dsp.context.urltoken.stok = nil
	end

	luci.http.header("Set-Cookie", "sysauth=; path=" .. dsp.build_url())
	luci.http.redirect(luci.dispatcher.build_url())
end

function lan_proc()
	local lan_ipReq   = luci.http.formvalue("lan_ip")
	local lan_maskReq = luci.http.formvalue("lan_mask")
	local old_lan_ipReq  = luci.http.formvalue("old_lan_ip")
	local old_lan_maskReq= luci.http.formvalue("old_lan_mask")

	local netmd = require "luci.model.network".init()
	local iface = "lan"
	local net = netmd:get_network(iface)	

	net:set("ipaddr",lan_ipReq)
	net:set("netmask",lan_maskReq)			
	--luci.sys.call("env -i /sbin/ifdown %q >/dev/null 2>/dev/null" % iface)
	netmd:commit("network")
	netmd:save("network")
	--luci.sys.call("env -i /sbin/ifup %q >/dev/null 2>/dev/null" % iface)

	luci.http.prepare_content("application/json")
	
	
	luci.http.write_json("[{lan_ip:'" .. lan_ipReq .. "',lan_mask:'" .. lan_maskReq .. "'}]")
	
	luci.sys.call("env -i /sbin/reboot & >/dev/null 2>/dev/null")
end
