#
#
hiwifi_root = $(shell pwd)
openwrt_dir = openwrt-ar71xx
packages_required = build-essential git flex gettext libncurses5-dev \
  unzip gawk liblzma-dev u-boot-tools rsync
openwrt_feeds = libevent2 luci luci-app-radvd luci-app-samba tayga pptpd curl

final: s_build_openwrt
	make -C tw150v1

s_build_openwrt: s_sync_files
# 7. Always replace .config with the repository's
	@cd $(openwrt_dir); \
		if [ -e .config ]; then \
			mv -vf .config .config.bak; \
			echo "WARNING: .config is updated, backed up as '.config.bak'"; \
		fi; \
		cp -vf ../config-openwrt-ar71xx-ap83 .config
####
	make -C $(openwrt_dir) V=s -j8
	@touch s_build_openwrt

clean:
	make clean -C tw150v1
	rm -f s_build_openwrt s_sync_files s_install_feeds
	make clean -C $(openwrt_dir) V=s

remake:
	@make clean -C tw150v1
	@rm -f s_build_openwrt s_sync_files s_install_feeds
	@make final

s_sync_files: s_install_feeds
	rsync -av --exclude=.svn files/ $(openwrt_dir)/
	@kernel_dir=`echo $(openwrt_dir)/build_dir/target-mips_*/linux-ar71xx_generic/linux-3.*`; \
		[ -d "$$kernel_dir" ] && rsync -av --exclude=.svn files/target/linux/ar71xx/files/ "$$kernel_dir/" || : ;
	@touch s_sync_files

s_install_feeds: s_update_feeds
	@cd $(openwrt_dir); ./scripts/feeds install $(openwrt_feeds);
	@touch s_install_feeds

s_update_feeds: s_checkout_svn
	@cd $(openwrt_dir); ./scripts/feeds update;
	@touch s_update_feeds

# 2. Checkout source code (this is the latest stable version recommended by OpenWrt Wiki):
s_checkout_svn: s_check_hostdeps
	svn co svn://svn.openwrt.org/openwrt/trunk $(openwrt_dir) -r38140
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

menuconfig: s_sync_files
	@cd $(openwrt_dir); [ -f .config ] && mv -vf .config .config.bak || :
	@cp -vf config-openwrt-ar71xx-ap83 $(openwrt_dir)/.config
	@touch config-openwrt-ar71xx-ap83  # change modification time
	@make -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt config-openwrt-ar71xx-ap83 ] && cp -vf $(openwrt_dir)/.config config-openwrt-ar71xx-ap83 || :

