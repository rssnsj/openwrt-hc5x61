#
#
KDIR = $(shell ls -d ../openwrt-ramips/build_dir/target-mipsel_*)/linux-ramips_mt7620a
HOSTBIN = ../openwrt-ramips/staging_dir/host/bin
SYSUPGRADE_BIN = openwrt-HC5761-sysupgrade.bin
FINAL_BIN = openwrt-HC5761-recovery.bin

$(FINAL_BIN): vmlinux root.squashfs
	$(HOSTBIN)/patch-cmdline vmlinux "board=HC5761 console=ttyS1,115200"
	$(HOSTBIN)/lzma e vmlinux -lc1 -lp2 -pb2 vmlinux-HC5761.bin.lzma
	$(HOSTBIN)/mkimage -A mips -O linux -T kernel -C lzma -a 0x80000000 -e 0x80000000 \
	  -n "HC5761"  -d vmlinux-HC5761.bin.lzma \
	  vmlinux-HC5761.uImage

	@( \
	  dd if=vmlinux-HC5761.uImage bs=1M conv=sync || exit 1; \
	  dd if=root.squashfs || exit 1; \
	) > $(SYSUPGRADE_BIN) || ( rm -vf $(SYSUPGRADE_BIN); exit 1 );

	@if [ `stat -c%s $(SYSUPGRADE_BIN)` -gt 16318464 ]; then \
	  echo "Warning: sysup img is too big"; \
	  exit 1; \
	fi

	@( \
	  dd if=openwrt-HC5761-uboot.bin bs=327680 conv=sync || exit 1; \
	  dd if=$(SYSUPGRADE_BIN) || exit 1; \
	) > $@ || ( rm -vf $@; exit 1 );

	ln -sf $@ recovery.bin

vmlinux: $(KDIR)/vmlinux-mt7620a
	cp -vf $< $@

root.squashfs: $(KDIR)/root.squashfs
	cp -vf $< $@

clean:
	rm -vf vmlinux root.squashfs vmlinux-HC5761.bin.lzma vmlinux-HC5761.uImage $(SYSUPGRADE_BIN) $(FINAL_BIN) recovery.bin

