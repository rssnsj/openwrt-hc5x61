#
#
KDIR = $(shell ls -d ../openwrt-ramips/build_dir/target-mipsel_*)/linux-ramips_mt7620a
HOSTBIN = ../openwrt-ramips/staging_dir/host/bin

all: openwrt-HC5761-recovery.bin openwrt-HC5661-recovery.bin
	ln -sf $< recovery.bin
	@echo ""
	@echo ">>>> Final images in 'recovery.bin' format:"
	@echo ">>>> $^"

openwrt-HC5661-recovery.bin: vmlinux-mt7620a root.squashfs
#	$(HOSTBIN)/patch-cmdline vmlinux-mt7620a "board=HC5661 console=ttyS1,115200"
	$(HOSTBIN)/lzma e vmlinux-mt7620a -lc1 -lp2 -pb2 vmlinux-HC5661.bin.lzma
	$(HOSTBIN)/mkimage -A mips -O linux -T kernel -C lzma -a 0x80000000 -e 0x80000000 \
		-n "HC5661"  -d vmlinux-HC5661.bin.lzma vmlinux-HC5661.uImage

	cat vmlinux-HC5661.uImage root.squashfs > openwrt-HC5661-sysupgrade.bin
	$(HOSTBIN)/padjffs2 openwrt-HC5661-sysupgrade.bin 4 8 16 64 128 256

	@if [ `stat -c%s openwrt-HC5661-sysupgrade.bin` -gt 16318464 ]; then \
	  echo "Warning: sysupgrade image is too big"; \
	  exit 1; \
	fi

	@( \
	  dd if=openwrt-HC5661-uboot.bin bs=327680 conv=sync || exit 1; \
	  dd if=openwrt-HC5661-sysupgrade.bin || exit 1; \
	) > $@ || ( rm -vf $@; exit 1 );

openwrt-HC5761-recovery.bin: vmlinux-mt7620a root.squashfs
#	$(HOSTBIN)/patch-cmdline vmlinux-mt7620a "board=HC5761 console=ttyS1,115200"
	$(HOSTBIN)/lzma e vmlinux-mt7620a -lc1 -lp2 -pb2 vmlinux-HC5761.bin.lzma
	$(HOSTBIN)/mkimage -A mips -O linux -T kernel -C lzma -a 0x80000000 -e 0x80000000 \
		-n "HC5761"  -d vmlinux-HC5761.bin.lzma vmlinux-HC5761.uImage

	cat vmlinux-HC5761.uImage root.squashfs > openwrt-HC5761-sysupgrade.bin
	$(HOSTBIN)/padjffs2 openwrt-HC5761-sysupgrade.bin 4 8 16 64 128 256

	@if [ `stat -c%s openwrt-HC5761-sysupgrade.bin` -gt 16318464 ]; then \
	  echo "Warning: sysupgrade image is too big"; \
	  exit 1; \
	fi

	@( \
	  dd if=openwrt-HC5761-uboot.bin bs=327680 conv=sync || exit 1; \
	  dd if=openwrt-HC5761-sysupgrade.bin || exit 1; \
	) > $@ || ( rm -vf $@; exit 1 );

vmlinux-mt7620a: $(KDIR)/vmlinux-hiwifi-hc5761
	cp -vf $< $@

root.squashfs: $(KDIR)/root.squashfs
	cp -vf $< $@

clean:
	rm -vf vmlinux-mt7620a root.squashfs vmlinux-HC5?61.bin.lzma vmlinux-HC5?61.uImage *-sysupgrade.bin *recovery.bin

