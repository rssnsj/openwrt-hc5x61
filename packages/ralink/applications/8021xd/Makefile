#
# Copyright (C) 2010-2013 hua.shao@mediatek.com
#
# Ralink Property Software.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=8021xd
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_KCONFIG:=RALINK_MT7620 RALINK_MT7621 RALINK_MT7628
PKG_CONFIG_DEPENDS:=$(foreach c, $(PKG_KCONFIG),$(if $(CONFIG_$c),CONFIG_$(c)))


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/kernel.mk

define Package/8021xd
  SECTION:=Ralink Properties
  CATEGORY:=Ralink Properties
  TITLE:=802.1X Daemon
  SUBMENU:=Applications
endef

define Package/8021xd/description
  802.1X Daemon
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

#TARGET_CFLAGS += \
#	-I$(LINUX_DIR)/include

MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS)"

define Package/8021xd/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/8021xd $(1)/bin/8021xd
	ln $(1)/bin/8021xd $(1)/bin/8021xdi
endef


$(eval $(call BuildPackage,8021xd))

