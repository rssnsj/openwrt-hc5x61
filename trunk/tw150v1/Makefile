#

openwrt-ar71xx-generic-hiwifi-hc6361-squashfs-factory.bin: prefix-1-2.bin openwrt-ar71xx-generic-hiwifi-hc6361-squashfs-sysupgrade.bin
	cat $^ > $@
	ln -sf $@ recovery.bin

openwrt-ar71xx-generic-hiwifi-hc6361-squashfs-sysupgrade.bin:
	wget http://downloads.openwrt.org/snapshots/trunk/ar71xx/openwrt-ar71xx-generic-hiwifi-hc6361-squashfs-sysupgrade.bin -O $@

clean:
	@rm -vf openwrt-ar71xx-generic-hiwifi-hc6361-*.bin

