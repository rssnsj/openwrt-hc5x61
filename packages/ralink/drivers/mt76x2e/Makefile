# All rights reserved.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=mt76x2e
PKG_REV:=5
PKG_VERSION:=$(PKG_REV)
PKG_RELEASE:=20150604

PKG_SOURCE_PROTO:=svn
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/i80s/mtk-sources/trunk/mt76x2e
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2

PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_KCONFIG:= \
	RALINK_MT7612E RALINK_MT7620 RLT_AP_SUPPORT WDS_SUPPORT MBSS_SUPPORT ENHANCE_NEW_MBSSID_MODE \
	APCLI_SUPPORT DFS_SUPPORT LLTD_SUPPORT NINTENDO_AP COC_SUPPORT \
	DELAYED_TCP_ACK_SUPPORT RT_FIRST_CARD_EEPROM \
	RT_FIRST_IF_RF_OFFSET RT_SECOND_IF_RF_OFFSET \
	RT_FIRST_CARD RT_SECOND_CARD SNIFFER_SUPPORT CFG80211_SUPPORT \
	RT_FIRST_CARD_EEPROM FIRST_IF_EEPROM_PROM FIRST_IF_EEPROM_EFUSE FIRST_IF_EEPROM_FLASH \
	SECOND_IF_EEPROM_PROM SECOND_IF_EEPROM_EFUSE SECOND_IF_EEPROM_FLASH \
	WIFI_BASIC_FUNC WSC_INCLUDED WSC_V2_SUPPORT WSC_NFC_SUPPORT DOT11N_DRAFT3 DOT11_VHT_AC \
	DOT11W_PMF_SUPPORT TXBF_SUPPORT LLTD_SUPPORT QOS_DLS_SUPPORT \
	CARRIER_DETECTION_SUPPORT IGMP_SNOOP_SUPPORT BLOCK_NET_IF \
	TXBF_SUPPORT RATE_ADAPTION NEW_RATE_ADAPT_SUPPORT AGS_SUPPORT \
	IDS_SUPPORT WIFI_WORK_QUEUE WIFI_SKB_RECYCLE RTMP_FLASH_SUPPORT \
	LED_CONTROL_SUPPORT HW_ANTENNA_DIVERSITY ATE_SUPPORT \
	RT2860V2_AP_V24_DATA_STRUCTURE RT2860V2_AP_32B_DESC MEMORY_OPTIMIZATION \
	RTMP_INTERNAL_TX_ALC RTMP_TEMPERATURE_CALIBRATION HOTSPOT \
	SINGLE_SKU_V2 RLT_MAC RLT_BBP RLT_RF RTMP_MAC RTMP_BBP RTMP_RF \
	RTMP_PCI_SUPPORT RTMP_USB_SUPPORT RTMP_RBUS_SUPPORT DISABLE_EDCCA MULTI_CORE_SUPPORT

PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))

include $(INCLUDE_DIR)/package.mk

define KernelPackage/mt76x2e
  CATEGORY:=Ralink Properties
  TITLE:=Ralink mt76x2e wifi AP driver
  FILES:=$(PKG_BUILD_DIR)/build/mt76x2e.ko
  DEPENDS:=+kmod-cfg80211 +uci2dat +8021xd
  AUTOLOAD:=$(call AutoLoad,91,mt76x2e)
  SUBMENU:=Drivers
  MENU:=1
endef

define KernelPackage/mt76x2e/config
	source "$(SOURCE)/config.in"
endef

define P4/Info
  P4URL:=//Embedded_WIFI/MP/MT76x2/MT7612e/AP/rlt_wifi/
endef

define P4/Release
  make release
  mkdir -p build
  cp os/linux/Makefile.rlt_wifi_ap build/Makefile
  cp os/linux/Kconfig.rlt_wifi_ap build/Kconfig
  mv DPA src
  tar cjf mt76x2e.tar.bz2 build src
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(LINUX_KARCH)" \
		SUBDIRS="$(PKG_BUILD_DIR)/build/" \
		$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_MT76X2E_$c),CONFIG_$(c)=$(CONFIG_MT76X2E_$c)))\
		CONFIG_SUPPORT_OPENWRT=y \
		modules
endef

define KernelPackage/mt76x2e/install
	$(INSTALL_DIR) $(1)/lib/wifi/
	echo ------------------------------------------------------------------- $(CONFIG_RT_SECOND_CARD)
#	if [ "$$(CONFIG_MT76X2E_RT_FIRST_CARD)" = "7602" ] || [ "$$(CONFIG_MT76X2E_RT_FIRST_CARD)" = "7602e" ]; then \
#		$(INSTALL_DIR) $(1)/etc/wireless/mt7602e/ ; \
#		$(INSTALL_BIN) ./files/mt7602e.sh $(1)/lib/wifi/ ; \
#		$(INSTALL_BIN) ./files/mt7602e.dat $(1)/etc/wireless/mt7602e/ ; \
#		$(INSTALL_BIN) ./files/mt7602e*.bin $(1)/etc/wireless/mt7602e/ ; \
#		$(INSTALL_BIN) ./files/SingleSKU.dat $(1)/etc/wireless/mt7602/SingleSKU.dat ; \
#		echo p$(P4REV) > $(1)/etc/wireless/mt7602e/version; \
#	fi
	if [ "$$(CONFIG_MT76X2E_RT_SECOND_CARD)" = "7612" ] || [ "$$(CONFIG_MT76X2E_RT_SECOND_CARD)" = "7612e" ]; then \
		$(INSTALL_DIR) $(1)/etc/wireless/mt7612e/ ; \
		$(INSTALL_BIN) ./files/mt7612e.sh $(1)/lib/wifi/ ; \
		$(INSTALL_BIN) ./files/ralink_common.sh $(1)/lib/wifi/ ; \
		$(INSTALL_BIN) ./files/mt7612e.dat $(1)/etc/wireless/mt7612e/ ; \
		$(INSTALL_BIN) ./files/mt7612e*.bin $(1)/etc/wireless/mt7612e/ ; \
		$(INSTALL_BIN) ./files/SingleSKU.dat $(1)/etc/wireless/mt7612/SingleSKU.dat ; \
		echo p$(P4REV) > $(1)/etc/wireless/mt7612e/version; \
	fi
endef

$(eval $(call KernelPackage,mt76x2e))
