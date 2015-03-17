# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mt7620
P4REV:=113050
PKG_VERSION:=p4rev-$(P4REV)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://localhost/
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_KCONFIG:=RALINK_MT7620 RALINK_RT5350 RALINK_3883 RALINK_RT3352 RT2860V2_AP RT2860V2_AP_V24_DATA_STRUCTURE \
	RT2860V2_AP_WSC RT2860V2_AP_WSC_V2 RT2860V2_AP_LLTD RT2860V2_AP_WDS RT2860V2_AP_MBSS NEW_MBSSID_MODE \
	RT2860V2_AP_APCLI RT2860V2_AP_MAC_REPEATER RT2860V2_AP_IGMP_SNOOP RT2860V2_AP_NETIF_BLOCK RT2860V2_AP_DFS \
	RT2860V2_AP_CARRIER RT2860V2_AP_DLS RT2860V2_AP_IDS RT2860V2_HW_ANTENNA_DIVERSITY RT2860V2_AP_COC \
	RT2860V2_AP_MEMORY_OPTIMIZATION RT2860V2_AP_VIDEO_TURBINE RA_CLASSIFIER RT2860V2_AP_INTELLIGENT_RATE_ADAPTION \
	RT2860V2_AP_TXBF RT2860V2_AP_80211N_DRAFT3 RT2860V2_ADJ_PWR_CONSUMPTION_SUPPORT RT2860V2_SINGLE_SKU \
	RT2860V2_AP_RTMP_INTERNAL_TX_ALC RT2860V2_AP_RTMP_TEMPERATURE_COMPENSATION RT2860V2_AP_LED \
	INTERNAL_PA_INTERNAL_LNA INTERNAL_PA_EXTERNAL_LNA EXTERNAL_PA_EXTERNAL_LNA RT2860V2_EXT_CHANNEL_LIST RT2860V2_AP_EDCCA_MONITOR
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk

TAR_CMD=$(HOST_TAR) -C $(1)/ $(TAR_OPTIONS)

define KernelPackage/mt7620
  CATEGORY:=Ralink Properties
  TITLE:=Ralink MT7620 wifi AP driver
  FILES:=$(PKG_BUILD_DIR)/build/mt7620.ko
  DEPENDS:=@TARGET_ramips_mt7620a
  AUTOLOAD:=$(call AutoLoad,90,mt7620)
  SUBMENU:=Drivers
  MENU:=1
endef

define KernelPackage/mt7620/config
	source "$(SOURCE)/config.in"
endef

define P4/Info
  P4URL:=//Embedded_WIFI/MP/MT7620/WIFI_MT7620/
endef

define P4/Release
  make release
  mkdir -p build
  cp rt2860v2_ap/Makefile build/Makefile
  cp rt2860v2_ap/Kconfig build/Kconfig
  mv ARCH src
  tar cjf mt7620.tar.bz2 build src
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)/build/" \
		$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)=y))\
		CONFIG_SUPPORT_OPENWRT=y \
		modules
endef

define KernelPackage/mt7620/install
	$(INSTALL_DIR) $(1)/lib/wifi/
	$(INSTALL_BIN) ./files/mt7620.sh $(1)/lib/wifi/
	$(INSTALL_DIR) $(1)/etc/wireless/mt7620/
	$(INSTALL_BIN) ./files/mt7620.dat $(1)/etc/wireless/mt7620/
	$(INSTALL_BIN) ./files/SingleSKU.dat $(1)/etc/wireless/mt7620/
	-if [ "$$(CONFIG_INTERNAL_PA_INTERNAL_LNA)" = "y" ]; then \
		$(INSTALL_BIN) ./files/mt7620.eeprom.ipa.ilna.bin $(1)/etc/wireless/mt7620/mt7620.eeprom.bin; \
	elif [ "$$(CONFIG_INTERNAL_PA_EXTERNAL_LNA)" = "y" ]; then \
		$(INSTALL_BIN) ./files/mt7620.eeprom.ipa.elna.bin $(1)/etc/wireless/mt7620/mt7620.eeprom.bin; \
	elif [ "$$(CONFIG_EXTERNAL_PA_EXTERNAL_LNA)" = "y" ]; then \
		$(INSTALL_BIN) ./files/mt7620.eeprom.epa.elna.bin $(1)/etc/wireless/mt7620/mt7620.eeprom.bin; \
	else \
		$(INSTALL_BIN) ./files/mt7620.eeprom.ipa.elna.bin $(1)/etc/wireless/mt7620/mt7620.eeprom.bin; \
	fi
	echo p$(P4REV) > $(1)/etc/wireless/mt7620/version
endef

$(eval $(call KernelPackage,mt7620))
