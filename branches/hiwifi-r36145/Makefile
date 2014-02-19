#
#
# WAN port as one of LAN ports
WAL ?= n
hiwifi_root = $(shell pwd)
root_squashfs = openwrt-ar71xx-generic-root.squashfs
target_fw = openwrt-tw150v1-recovery.bin
openwrt_svn = svn://svn.openwrt.org/openwrt/trunk
openwrt_rev = 38140
openwrt_dir = openwrt-ar9331
mach_to_replace = $(openwrt_dir)/target/linux/ar71xx/files/arch/mips/ath79/mach-ap83.c
packages_required = build-essential git flex gettext libncurses5-dev \
  unzip gawk liblzma-dev u-boot-tools 

final: s_build_openwrt
	./firmware-builder/tw150v1-buildfw.sh "$(hiwifi_root)/$(openwrt_dir)" "$(target_fw)" \
		&& mv -vf "./firmware-builder/$(target_fw)" ./

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
# Set the HIWIFI_WAN_AS_LAN_PORT macro in mach-ap83.c
	@if [ $(WAL) = y ]; then \
		sed -i 's/^\/\/\(#define[ \t]\+HIWIFI_WAN_AS_LAN_PORT.*\)$$/\1/' mach-tw150v1/mach-ap83.c; \
	else \
		sed -i 's/^\(#define[ \t]\+HIWIFI_WAN_AS_LAN_PORT.*\)$$/\/\/\1/' mach-tw150v1/mach-ap83.c; \
	fi;
# 5. Add 'tw150v1' platform support: <<<< IMPORTANT
	@if ! [ -L $(mach_to_replace) -a -e $(mach_to_replace) ]; then \
		rm -vf $(mach_to_replace); \
		ln -sv "$(hiwifi_root)/mach-tw150v1/mach-ap83.c" $(mach_to_replace); \
	fi;
# 5. Modify the firmware tools to allow image size up to 8M
	@cd $(openwrt_dir); \
		sed -i 's#^\(.*tl-wr703n-v1.*\)4Mlzma\(.*\)$$#\18Mlzma\2#g' target/linux/ar71xx/image/Makefile; \
		sed -i 's#^\(.*tl-wr720n-v3.*\)4Mlzma\(.*\)$$#\18Mlzma\2#g' target/linux/ar71xx/image/Makefile; \
		for mid in HWID_TL_WR703N_V1 HWID_TL_WR720N_V3; do \
			mline=`awk '/hw_id.*=.*'$$mid'/{print NR+2}' tools/firmware-utils/src/mktplinkfw.c`; \
			[ -z "$$mline" ] && continue || : ; \
			sed -i $$mline's#4Mlzma#8Mlzma#' tools/firmware-utils/src/mktplinkfw.c; \
		done;
# 7. Always replace .config with the repository's
	@cd $(openwrt_dir); \
		if [ -e .config ]; then \
			mv -vf .config .config.bak; \
			echo "WARNING: .config is updated, backed up as '.config.bak'"; \
		fi; \
		cp -vf ../config-openwrt-ar9xxx-ap83 .config
#
	make -C $(openwrt_dir) V=s

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
	@[ -d $(openwrt_dir) ] && svn up $(openwrt_dir) -r$(openwrt_rev) || \
		svn co $(openwrt_svn) $(openwrt_dir) -r$(openwrt_rev)
	@touch s_checkout_svn

menuconfig:
	@cd $(openwrt_dir); mv -vf .config .config.bak
	@cp -vf config-openwrt-ar9xxx-ap83 $(openwrt_dir)/.config
	@touch config-openwrt-ar9xxx-ap83
	@make -C $(openwrt_dir) menuconfig
	@[ $(openwrt_dir)/.config -nt config-openwrt-ar9xxx-ap83 ] && cp -vf $(openwrt_dir)/.config config-openwrt-ar9xxx-ap83 || :

