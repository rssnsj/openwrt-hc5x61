
BUILDER ?= OpenWrt-ImageBuilder-15.05-ramips-mt7628.Linux-x86_64
SDK ?= OpenWrt-SDK-15.05-ramips-mt7628_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64

OPKG_PACKAGES :=  6in4 6to4 blkid curl ethtool fdisk ip ip6tables-extra ip6tables-mod-nat \
	ipset iptables-mod-conntrack-extra iptables-mod-extra iptables-mod-ipopt iwinfo \
	iptables-mod-nat-extra kmod-crypto-des kmod-crypto-ecb kmod-crypto-hmac kmod-crypto-manager \
	kmod-crypto-md4 kmod-crypto-md5 kmod-crypto-pcompress kmod-crypto-sha1 kmod-crypto-sha256 \
	kmod-dnsresolver kmod-fs-ext4 kmod-fs-vfat kmod-fs-cifs kmod-fuse kmod-gre kmod-ipt-filter\
	kmod-ip6-tunnel kmod-ip6tables-extra kmod-ipip kmod-ipt-conntrack-extra kmod-ipt-extra \
	kmod-ipt-ipopt kmod-ipt-ipset kmod-ipt-nat-extra kmod-ipt-nat6 kmod-iptunnel kmod-iptunnel4 \
	kmod-iptunnel6 kmod-leds-gpio kmod-ledtrig-default-on kmod-ledtrig-netdev kmod-lib-zlib \
	kmod-ledtrig-timer kmod-ledtrig-usbdev kmod-lib-textsearch kmod-macvlan kmod-mppe \
	kmod-nfnetlink kmod-pptp kmod-sit kmod-tun kmod-usb-storage libblkid libcurl libdaemon \
	libevent2 liblzo libmnl libncurses libopenssl libpolarssl libpthread luci-app-samba \
	luci-lib-json luci-proto-ipv6 luci-proto-relay ntfs-3g openvpn-openssl resolveip \
	samba36-server terminfo zlib luci-i18n-base-zh-cn luci-i18n-commands-zh-cn \
	luci-i18n-diag-core-zh-cn luci-i18n-firewall-zh-cn luci-i18n-qos-zh-cn luci-i18n-samba-zh-cn \
	ppp-mod-pptp ppp-mod-pppol2tp kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 \
	kmod-crypto-deflate kmod-usb-ohci kmod-usb-printer kmod-nf-nathelper kmod-nf-nathelper-extra
OPKG_PACKAGES_HC5661A :=
PREPARED_TARGETS = .check_ib .patched

# A single option for enabling all options
ifeq ($(FULL),1)
	RALINK := 1
	FEEDS := 1
	WIFI := 1
endif
# Check each option for selection of packages and dependencies
ifeq ($(RALINK),1)
	OPKG_PACKAGES_HC5661A += 8021xd uci2dat kmod-mt7610e luci-mtk-wifi
	PREPARED_TARGETS += .check_sdk .ralink
endif
ifeq ($(FEEDS),1)
	OPKG_PACKAGES += ipset-lists minivtun shadowsocks-libev shadowsocks-tools dnsmasq-full \
		file-storage dnspod-utils kmod-proto-bridge kmod-yavlan
	PREPARED_TARGETS += .check_sdk .feeds
endif

define BeforeBuildImage
	mkdir -p $(BUILDER)/dl
	cp -f repo-base.conf $(BUILDER)/repositories.conf
	@[ -n "$(SDK)" -a -f "$(SDK)"/bin/ramips/packages/Packages ] && \
		echo "src ralink file:$(shell cd $(SDK)/bin/ramips/packages; pwd)" >> $(BUILDER)/repositories.conf || :
	mkdir -p $(BUILDER)/target/linux/ramips/base-files/etc
	cp -f opkg.conf $(BUILDER)/target/linux/ramips/base-files/etc/opkg.conf
endef

define EnableWireless
	@[ -n "$(WIFI)" ] || exit 0; \
		F="$(1)"; openwrt-repack.sh $$F -w -o $$F.out && mv $$F.out $$F
endef

HC5661A: $(PREPARED_TARGETS)
	$(call BeforeBuildImage)
	make -C $(BUILDER) image PROFILE=HiWiFi-HC5661A \
		FILES="$(shell cd $(BUILDER); pwd)/target/linux/ramips/base-files" \
		PACKAGES="$(OPKG_PACKAGES) $(OPKG_PACKAGES_HC5661A)"
	$(call EnableWireless,$(shell echo $(BUILDER)/bin/ramips/openwrt*-ramips-*-hc5661a-*-sysupgrade.bin))

.patched:
	cat ../patches/01-hiwifi-hc5x61.patch ../patches/12-disable-pdnsd.patch > .patching
	patch -d $(BUILDER) -p0 < .patching
	mv .patching .patched

.ralink:
	@cd $(SDK); [ ! -L dl -a -d /var/dl ] && { rmdir dl && ln -s /var/dl; } || :
	@cd $(SDK)/package; [ -d ralink ] || ln -sv $(shell cd ..; pwd)/packages/ralink
	make package/8021xd/compile V=s -C "$(SDK)"
	make package/uci2dat/compile V=s -C "$(SDK)"
	make package/mt7610e/compile V=s -C "$(SDK)"
	make package/mt76x2e/compile V=s -C "$(SDK)"
	make package/luci-mtk-wifi/compile V=s -C "$(SDK)"
	cd "$(SDK)/bin/ramips/packages" && ../../../scripts/ipkg-make-index.sh . > Packages && gzip -9c Packages > Packages.gz

.feeds:
	@cd $(SDK); [ ! -L dl -a -d /var/dl ] && { rmdir dl && ln -s /var/dl; } || :
	@cd $(SDK)/package; [ -d network-feeds ] && { cd network-feeds; git pull; } || git clone https://github.com/rssnsj/network-feeds.git
	make package/ipset-lists/compile V=s -C "$(SDK)"
	make package/shadowsocks-libev/compile V=s -C "$(SDK)"
	make package/shadowsocks-tools/compile V=s -C "$(SDK)"
	make package/minivtun-tools/compile V=s -C "$(SDK)"
	make package/file-storage/compile V=s -C "$(SDK)"
	make package/dnspod-utils/compile V=s -C "$(SDK)"
	make package/proto-bridge/compile V=s -C "$(SDK)"
	cd "$(SDK)/bin/ramips/packages" && ../../../scripts/ipkg-make-index.sh . > Packages && gzip -9c Packages > Packages.gz

.check_ib:
	@if ! [ -n "$(BUILDER)" -a -d "$(BUILDER)" ]; then \
		echo "Please specify a valid ImageBuilder directory by adding \"BUILDER=...\"."; \
		echo "Type \"make help\" for more details."; \
		exit 1; \
	fi
.check_sdk:
	@if ! [ -n "$(SDK)" -a -d "$(SDK)/package" ]; then \
		echo "Please specify a valid OpenWrt SDK directory by adding \"SDK=...\"."; \
		echo "Type \"make help\" for more details."; \
		exit 1; \
	fi

# Try extracting ImageBuilder & SDK to current directory
extract:
	[ -d "$(BUILDER)" ] || tar jxvf /var/dl/$(BUILDER).tar.bz2
	[ -d "$(SDK)" ] || tar jxvf /var/dl/$(SDK).tar.bz2

help:
	@echo "Usage:"
	@echo "  make BUILDER=.... [RALINK=1] [FEEDS=1]     build OpenWrt firmware for this board"
	@echo "Options:"
	@echo "  BUILDER=<directory>        specify a valid ImageBuilder directory"
	@echo "  SDK=<directory>            specify a valid OpenWrt SDK directory"
	@echo "  RALINK=1                   build and install Ralink 5G drivers"
	@echo "  FEEDS=1                    build and install Shadowsocks, minivtun, kmod-proto-bridge, kmod-yavlan"
	@echo "  FULL=1                     enable both the options above"

clean: .check_ib
	make clean -C $(BUILDER)
	@if [ -e .patched ]; then \
		patch -R -d $(BUILDER) -p0 < .patched; \
		rm -vf .patched; \
	fi
	[ -n "$(SDK)" -a -d "$(SDK)"/bin/ramips ] && rm -rf "$(SDK)"/bin/ramips/* || :
