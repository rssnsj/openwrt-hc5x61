
BUILDER := OpenWrt-ImageBuilder-ramips_mt7620a-for-linux-x86_64
SDK := OpenWrt-SDK-ramips-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2
FULL ?= 1

EXTRA_PACKAGES := kmod-tun \
	iptables-mod-conntrack-extra iptables-mod-extra iptables-mod-filter \
	iptables-mod-nat-extra ip6tables-extra ip6tables-mod-nat \
	ip ipset ppp-mod-pppol2tp ppp-mod-pptp xl2tpd pptpd curl ethtool \
	luci-i18n-chinese luci-i18n-english

all: HC5661 HC5761 HC5861

define BeforeBuildImage
	mkdir -p $(BUILDER)/dl
	cp -f repo-base.conf $(BUILDER)/repositories.conf
	@[ -n "$(SDK)" -a -f "$(SDK)"/bin/ramips/packages/Packages ] && \
		echo "src ralink file:$(shell cd $(SDK)/bin/ramips/packages; pwd)" >> $(BUILDER)/repositories.conf || :
	mkdir -p $(BUILDER)/target/linux/ramips/base-files/etc
	cp -f opkg.conf $(BUILDER)/target/linux/ramips/base-files/etc/opkg.conf
endef

HC5661: patch_ib
	$(call BeforeBuildImage)
	$(MAKE) -C $(BUILDER) image PROFILE=HC5661 \
		FILES="$(shell cd $(BUILDER) && pwd)/target/linux/ramips/base-files" \
		PACKAGES="$(EXTRA_PACKAGES)"

HC5761: patch_ib build_drivers
	$(call BeforeBuildImage)
	$(MAKE) -C $(BUILDER) image PROFILE=HC5761 \
		FILES="$(shell cd $(BUILDER) && pwd)/target/linux/ramips/base-files" \
		PACKAGES="$(EXTRA_PACKAGES) 8021xd uci2dat kmod-mt7610e luci-mtk-wifi"

HC5861: patch_ib build_drivers
	$(call BeforeBuildImage)
	$(MAKE) -C $(BUILDER) image PROFILE=HC5861 \
		FILES="$(shell cd $(BUILDER) && pwd)/target/linux/ramips/base-files" \
		PACKAGES="$(EXTRA_PACKAGES) 8021xd uci2dat kmod-mt76x2e luci-mtk-wifi"

build_drivers: $(SDK)
	$(MAKE) -C $(SDK) package/8021xd/compile package/uci2dat/compile package/mt7610e/compile package/mt76x2e/compile package/luci-mtk-wifi/compile V=s
	cd "$(SDK)/bin/ramips/packages" && ../../../scripts/ipkg-make-index.sh . > Packages && gzip -9c Packages > Packages.gz

patch_ib: $(BUILDER)
	@[ -f .patched ] && exit 0; \
		mkdir -p $(BUILDER)/target/linux/ramips/patches-3.10; \
		cat ../patches/01-hiwifi-hc5x61.patch > .patching; \
		patch -d $(BUILDER) -p0 < .patching && mv .patching .patched

# Try extracting ImageBuilder & SDK to current directory
$(SDK): /var/dl/$(SDK).tar.bz2
	tar axvf $< || { rm -rf $@; exit 1; }
	cd $(SDK) && [ ! -L dl -a -d /var/dl ] && { rmdir dl && ln -s /var/dl; } || :
	cd $(SDK)/package && { [ -d ralink ] || ln -sv ../../../packages/ralink; }
$(BUILDER): /var/dl/$(BUILDER).tar.bz2
	tar axvf $< || { rm -rf $@; exit 1; }

help:
	@echo "Usage:"
	@echo "  make HC5661|HC5761|HC5861"

clean:
	rm -f patching .patched
	rm -rf $(BUILDER) $(SDK)
