#
#
hiwifi_root = $(shell pwd)
openwrt_dir = openwrt-ramips
host_packages = build-essential git flex gettext libncurses5-dev unzip gawk liblzma-dev u-boot-tools
CONFIG_FILENAME = config

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
	$(MAKE) -C $(openwrt_dir) V=s

recovery.bin: HC5X61
	$(MAKE) -C recovery.bin

.install_feeds: .update_feeds
	@cd $(openwrt_dir)/package; \
	 [ -e extra-packages ] || ln -s ../../packages extra-packages
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
#	svn co svn://svn.openwrt.org/openwrt/branches/barrier_breaker $(openwrt_dir) -r43166 || :
	@git clone https://github.com/i80s/barrier_breaker.git $(openwrt_dir) && \
	 cd $(openwrt_dir) && git reset --hard b763ba211deeab857ef7c2e5275e92c15dd5e249
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
	@$(MAKE) -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt $(CONFIG_FILENAME) ] && cp -vf $(openwrt_dir)/.config $(CONFIG_FILENAME) || :

clean:
	$(MAKE) clean -C recovery.bin
	$(MAKE) clean -C $(openwrt_dir) V=s

