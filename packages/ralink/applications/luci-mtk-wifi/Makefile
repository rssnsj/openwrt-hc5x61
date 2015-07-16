#
# Copyright (c) 2015 Justin Liu
# Author: Justin Liu <rssnsj@gmail.com>
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-mtk-wifi
PKG_VERSION:=20150716
PKG_RELEASE:=

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/luci-mtk-wifi
  CATEGORY:=Network
  TITLE:=LuCI patch for MTK Wi-Fi drivers
  MAINTAINER:=Justin Liu <rssnsj@gmail.com>
  DEPENDS:=+luci-mod-admin-full
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile/Default

endef
Build/Compile = $(Build/Compile/Default)

define Package/luci-mtk-wifi/install
	$(CP) -a files/* $(1)/
endef

#define Package/luci-mtk-wifi/postinst
##!/bin/sh
#if [ -e /etc/openwrt_release ]; then
#	/etc/init.d/ss-redir.sh enable || :
#fi
#endef

$(eval $(call BuildPackage,luci-mtk-wifi))
