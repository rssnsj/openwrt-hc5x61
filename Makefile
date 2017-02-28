#
#
hiwifi_root = $(shell pwd)
openwrt_dir = openwrt-ramips
host_packages = build-essential git flex gettext libncurses5-dev unzip gawk liblzma-dev u-boot-tools
openwrt_feeds = libevent2 luci luci-app-samba xl2tpd pptpd pdnsd ntfs-3g ethtool
### mwan3 luci-app-mwan3
CONFIG_FILENAME = config-hiwifi

RUNNING_THREADS := $(shell cat /proc/cpuinfo | grep -i '^processor\s\+:' | wc -l)

define CheckConfigSymlink
	@if ! [ -e $(CONFIG_FILENAME) ]; then \
		echo "*** Please make a symbolic of either config file to '$(CONFIG_FILENAME)'."; \
		exit 1; \
	 fi
endef

HC5X61: .install_feeds
	@cd $(openwrt_dir); \
		if [ -e .config ]; then \
			mv -vf .config .config.bak; \
			echo "WARNING: .config is updated, backed up as '.config.bak'"; \
		fi
	$(call CheckConfigSymlink)
	cp -vf $(CONFIG_FILENAME) $(openwrt_dir)/.config
	@[ -f .config.extra ] && cat .config.extra >> $(openwrt_dir)/.config || :
	make -C $(openwrt_dir) V=s -j$(RUNNING_THREADS)

recovery.bin: HC5X61
	make -C recovery.bin

.install_feeds: .update_feeds
	@cd $(openwrt_dir); ./scripts/feeds install $(openwrt_feeds);
	@cd $(openwrt_dir)/package; \
	 [ -e rssnsj-packages ] || ln -s ../../packages rssnsj-packages; \
	 [ -e rssnsj-feeds ] || git clone https://github.com/rssnsj/network-feeds.git rssnsj-feeds
	@touch .install_feeds

.update_feeds: .patched
	@cd $(openwrt_dir); ./scripts/feeds update;
	@touch .update_feeds

.patched: .checkout_svn
	$(call CheckConfigSymlink)
	@cp -vf $(CONFIG_FILENAME) $(openwrt_dir)/.config
	@cd $(openwrt_dir); cat ../patches/*.patch | patch -p0
	@touch .patched

# 2. Checkout source code:
.checkout_svn: .check_hostdeps
	svn co svn://svn.openwrt.org/openwrt/branches/chaos_calmer $(openwrt_dir) -r46893
	@[ -d /var/dl ] && ln -sf /var/dl $(openwrt_dir)/dl || :
	@touch .checkout_svn

.check_hostdeps:
# 1. Install required host components:
	@which dpkg >/dev/null 2>&1 || exit 0; \
	for p in $(host_packages); do \
		dpkg -s $$p >/dev/null 2>&1 || to_install="$$to_install$$p "; \
	done; \
	if [ -n "$$to_install" ]; then \
		echo "Please install missing packages by running the following commands:"; \
		echo "  sudo apt-get update"; \
		echo "  sudo apt-get install -y $$to_install"; \
		exit 1; \
	fi;
	@touch .check_hostdeps

menuconfig: .install_feeds
	@cd $(openwrt_dir); [ -f .config ] && mv -vf .config .config.bak || :
	$(call CheckConfigSymlink)
	@cp -vf $(CONFIG_FILENAME) $(openwrt_dir)/.config
	@touch $(CONFIG_FILENAME)  # change modification time
	@make -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt $(CONFIG_FILENAME) ] && cp -vf $(openwrt_dir)/.config $(CONFIG_FILENAME) || :

clean:
	make clean -C recovery.bin
	make clean -C $(openwrt_dir) V=s

