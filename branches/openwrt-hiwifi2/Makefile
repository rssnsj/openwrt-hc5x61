#
#
# WAN port as one of LAN ports
WAL ?= n
hiwifi_root = $(shell pwd)
#root_squashfs = openwrt-ar71xx-generic-root.squashfs
#target_fw = openwrt-tw150v1-recovery.bin
openwrt_dir = openwrt-ramips
#mach_to_replace = $(openwrt_dir)/target/linux/ar71xx/files/arch/mips/ath79/mach-ap83.c
packages_required = build-essential git flex gettext libncurses5-dev \
  unzip gawk liblzma-dev u-boot-tools 

final: s_build_openwrt
#	./firmware-builder/tw150v1-buildfw.sh "$(hiwifi_root)/$(openwrt_dir)" "$(target_fw)" \
#		&& mv -vf "./firmware-builder/$(target_fw)" ./

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
# 7. Always replace .config with the repository's
	@cd $(openwrt_dir); \
		if [ -e .config ]; then \
			mv -vf .config .config.bak; \
			echo "WARNING: .config is updated, backed up as '.config.bak'"; \
		fi; \
		cp -vf ../config-openwrt-mt7620a .config
#
	make -C $(openwrt_dir) V=s

s_install_feeds: s_update_feeds
	@cd $(openwrt_dir); \
		./scripts/feeds install libevent2; \
		./scripts/feeds install luci; \
		./scripts/feeds install luci-app-radvd; \
		./scripts/feeds install luci-app-samba; \
		./scripts/feeds install rp-pppoe-server; \
		: ;
	@touch s_install_feeds

s_update_feeds: s_checkout_svn
	@cd $(openwrt_dir); ./scripts/feeds update;
	@touch s_update_feeds

# 2. Checkout source code (this is the latest stable version recommended by OpenWrt Wiki):
s_checkout_svn:
	svn co svn://svn.openwrt.org/openwrt/trunk $(openwrt_dir) -r38996
	@touch s_checkout_svn

menuconfig:
	@cd $(openwrt_dir); mv -vf .config .config.bak
	@cp -vf config-openwrt-mt7620a $(openwrt_dir)/.config
	@touch config-openwrt-mt7620a
	@make -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt config-openwrt-mt7620a ] && cp -vf $(openwrt_dir)/.config config-openwrt-mt7620a || :

