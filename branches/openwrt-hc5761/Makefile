#
#
hiwifi_root = $(shell pwd)
openwrt_dir = openwrt-ramips
packages_required = build-essential git flex gettext libncurses5-dev \
  unzip gawk liblzma-dev u-boot-tools rsync
openwrt_feeds = libevent2 luci luci-app-radvd luci-app-samba

final: s_hiwifi_patch
	@cd $(openwrt_dir); \
		if [ -e .config ]; then \
			mv -vf .config .config.bak; \
			echo "WARNING: .config is updated, backed up as '.config.bak'"; \
		fi; \
		cp -vf ../config-hiwifi-hc5761 .config
	make -C $(openwrt_dir) V=s -j4

clean:
	rm -f s_build_openwrt
	make clean -C recovery.bin
	make clean -C $(openwrt_dir) V=s

s_hiwifi_patch: s_install_feeds
	@cd $(openwrt_dir); patch -p0 < ../hiwifi-hc5761.patch
	@touch s_hiwifi_patch

s_install_feeds: s_update_feeds
	@cd $(openwrt_dir); ./scripts/feeds install $(openwrt_feeds);
	@touch s_install_feeds

s_update_feeds: s_checkout_svn
	@cd $(openwrt_dir); ./scripts/feeds update;
	@touch s_update_feeds

# 2. Checkout source code:
s_checkout_svn: s_check_hostdeps
	svn co svn://svn.openwrt.org/openwrt/trunk $(openwrt_dir) -r42871
	@[ -d /var/dl ] && ln -sf /var/dl $(openwrt_dir)/dl || :
	@touch s_checkout_svn

s_check_hostdeps:
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
	@touch s_check_hostdeps

menuconfig: s_hiwifi_patch
	@cd $(openwrt_dir); [ -f .config ] && mv -vf .config .config.bak || :
	@cp -vf config-hiwifi-hc5761 $(openwrt_dir)/.config
	@touch config-hiwifi-hc5761  # change modification time
	@make -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt config-hiwifi-hc5761 ] && cp -vf $(openwrt_dir)/.config config-hiwifi-hc5761 || :

