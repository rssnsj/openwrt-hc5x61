
BUILDER ?= OpenWrt-ImageBuilder-ramips_mt7620a-for-linux-x86_64
SDK ?= OpenWrt-SDK-ramips-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2

OPKG_PACKAGES := 6in4 6to4 blkid curl ethtool iftop ip ip6tables-extra ip6tables-mod-nat iperf-mt ipset \
	iptables-mod-conntrack-extra iptables-mod-extra iptables-mod-filter iptables-mod-ipopt iptables-mod-nat-extra \
	iwinfo kmod-crypto-deflate kmod-crypto-des kmod-crypto-ecb kmod-crypto-hmac kmod-crypto-manager \
	kmod-crypto-md4 kmod-crypto-md5 kmod-crypto-pcompress kmod-crypto-sha1 kmod-crypto-sha256 kmod-dnsresolver \
	kmod-fs-cifs kmod-fs-hfs kmod-fs-nfs kmod-fs-nfs-common kmod-fs-ntfs kmod-fs-vfat kmod-fuse kmod-gre kmod-ifb \
	kmod-ip6-tunnel kmod-ip6tables-extra kmod-ipip kmod-ipt-conntrack-extra kmod-ipt-extra kmod-ipt-filter \
	kmod-ipt-ipopt kmod-ipt-ipset kmod-ipt-nat-extra kmod-ipt-nat6 kmod-ipt-nathelper-extra kmod-iptunnel \
	kmod-iptunnel4 kmod-iptunnel6 kmod-l2tp kmod-ledtrig-gpio kmod-lib-textsearch kmod-lib-zlib kmod-macvlan \
	kmod-mppe kmod-nfnetlink kmod-nls-cp437 kmod-nls-iso8859-1 kmod-nls-utf8 kmod-pppol2tp kmod-pptp \
	kmod-sched-connmark kmod-sched-core kmod-sit kmod-tun kmod-usb-ohci kmod-usb-printer libcurl libdaemon \
	libevent2 libiwinfo libiwinfo-lua libjson liblua liblzo libmnl libncurses libopenssl libpcap libpolarssl \
	libpthread librt libubus-lua libuci-lua lua luci luci-app-firewall luci-app-qos luci-app-samba luci-base \
	luci-i18n-chinese luci-i18n-english luci-lib-json luci-lib-nixio luci-mod-admin-full luci-proto-ipv6 \
	luci-proto-ppp luci-proto-relay luci-theme-bootstrap ntfs-3g openvpn-openssl ppp-mod-pppol2tp ppp-mod-pptp \
	qos-scripts relayd resolveip samba36-server tc tcpdump terminfo uclibcxx uhttpd uhttpd-mod-ubus zlib \
	pdnsd pptpd xl2tpd
OPKG_REPOS := repo-base.conf

ifeq ($(FULL),1)
	OPKG_PACKAGES += ipset-lists minivtun shadowsocks-libev shadowsocks-tools dnsmasq-full
	OPKG_REPOS += repo-rssnsj.conf
endif

rom: .check_ib $(BUILDER)/.patched-hiwifi
	mkdir -p $(BUILDER)/dl
	cat $(OPKG_REPOS) > $(BUILDER)/repositories.conf
	make -C $(BUILDER) image PROFILE=HiWiFi-HC5761 \
		FILES="$(shell cd $(BUILDER); pwd)/target/linux/ramips/base-files" \
		PACKAGES="$(OPKG_PACKAGES)"

rom5g: .check_ib .ralink_built $(BUILDER)/.patched-hiwifi
	mkdir -p $(BUILDER)/dl
	cat $(OPKG_REPOS) > $(BUILDER)/repositories.conf
	echo "src ralink file:$(shell cd $(SDK)/bin/ramips/packages; pwd)" >> $(BUILDER)/repositories.conf
	make -C $(BUILDER) image PROFILE=HiWiFi-HC5761 \
		FILES="$(shell cd $(BUILDER); pwd)/target/linux/ramips/base-files" \
		PACKAGES="$(OPKG_PACKAGES) 8021xd uci2dat kmod-mt7610e"

ralink .ralink_built: .check_sdk
	@cd $(SDK); [ -d package/ralink ] || ln -sv $(shell pwd)/../packages/ralink package/ralink
	@cd $(SDK); [ -f .config.orig ] || mv -vf .config .config.orig
	@cd $(SDK); [ ! -L dl -a -d /var/dl ] && { rmdir dl && ln -s /var/dl; } || :
	cat "$(SDK)/.config.orig" config.ralink > "$(SDK)/.config"
	make package/mt7610e/compile V=s -C "$(SDK)"
	make package/8021xd/compile V=s -C "$(SDK)"
	make package/uci2dat/compile V=s -C "$(SDK)"
	cd "$(SDK)/bin/ramips/packages" && ../../../scripts/ipkg-make-index.sh . > Packages && gzip -9c Packages > Packages.gz
	touch .ralink_built

$(BUILDER)/.patched-hiwifi patch:
	mkdir -p $(BUILDER)/target/linux/ramips/patches-3.10
	cat ../patches/01-hiwifi-hc5761.patch ../patches/12-disable-pdnsd.patch | patch -d $(BUILDER) -p0
	touch $(BUILDER)/.patched-hiwifi

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

help:
	@echo "Usage:"
	@echo " make BUILDER=<ImageBuilder> [FULL=1]"
	@echo "Options:"
	@echo " Use \"BUILDER=<ImageBuilder>\" to specify a valid ImageBuilder directory."
	@echo " Use \"FULL=1\" to build fully functional firmware with Shadowsocks & minivtun."

clean: .check_ib
	make clean -C $(BUILDER)
	@if [ -e $(BUILDER)/.patched-hiwifi ]; then \
		rm -vf $(BUILDER)/.patched-hiwifi; \
		cat ../patches/01-hiwifi-hc5761.patch ../patches/12-disable-pdnsd.patch | patch -R -d $(BUILDER) -p0; \
	fi
	rm -f .ralink_built

