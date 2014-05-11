#
#
hiwifi_root = $(shell pwd)
openwrt_dir = openwrt-ramips
packages_required = build-essential git flex gettext libncurses5-dev \
  unzip gawk liblzma-dev u-boot-tools rsync


final: s_build_openwrt
	make -C hiwifi2

s_build_openwrt: s_install_feeds
# 1. Install required host components:
	@for p in $(packages_required); do \
		dpkg -s $$p &>/dev/null || to_install="$$to_install$$p "; \
	done; \
	if [ -n "$$to_install" ]; then \
		echo "Please install missing packages by running the following commands:"; \
		echo "  sudo apt-get update"; \
		echo "  sudo apt-get install -y $$to_install"; \
		exit 1; \
	fi;
	@rsync -rlpv --exclude=.svn files/ $(openwrt_dir)/
# 7. Always replace .config with the repository's
	@cd $(openwrt_dir); \
		if [ -e .config ]; then \
			mv -vf .config .config.bak; \
			echo "WARNING: .config is updated, backed up as '.config.bak'"; \
		fi; \
		cp -vf ../config-openwrt-mt7620a .config
#
	make -C $(openwrt_dir) V=s $(MAKEFLAGS) 2>&1 | tee build.log
	@touch s_build_openwrt

clean:
	rm -f s_build_openwrt
	make clean -C $(openwrt_dir) V=s

s_install_feeds: s_update_feeds
	@cd $(openwrt_dir); \
		./scripts/feeds install libevent2; \
		./scripts/feeds install luci; \
		./scripts/feeds install luci-app-radvd; \
		./scripts/feeds install luci-app-samba; \
		: ;
	@touch s_install_feeds

s_update_feeds: s_checkout_svn
	@cd $(openwrt_dir); ./scripts/feeds update;
	@touch s_update_feeds

# 2. Checkout source code (this is the latest stable version recommended by OpenWrt Wiki):
s_checkout_svn:
	svn co svn://svn.openwrt.org/openwrt/branches/attitude_adjustment $(openwrt_dir) -r40431
	@touch s_checkout_svn

menuconfig:
	@cd $(openwrt_dir); mv -vf .config .config.bak
	@cp -vf config-openwrt-mt7620a $(openwrt_dir)/.config
	@touch config-openwrt-mt7620a
	@make -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt config-openwrt-mt7620a ] && cp -vf $(openwrt_dir)/.config config-openwrt-mt7620a || :

