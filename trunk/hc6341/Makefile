#
#
root_squashfs = openwrt-ar71xx-generic-root.squashfs
openwrt_root = $(shell pwd)/../openwrt-ar71xx

build_tw150v1: prefix-1-2.bin
	cp $(openwrt_root)/build_dir/target-mips*/linux-ar71xx_generic/vmlinux vmlinux-hc6341
	$(openwrt_root)/staging_dir/host/bin/patch-cmdline  vmlinux-hc6341 "board=HC6341 console=ttyATH0,115200"
	$(openwrt_root)/staging_dir/host/bin/lzma  e vmlinux-hc6341 -lc1 -lp2 -pb2 vmlinux-hc6341.bin.lzma
	cp $(openwrt_root)/bin/ar71xx/$(root_squashfs) ./
	mkimage -A mips -O linux -T kernel -a 0x80060000 -C lzma -e 0x80060000 \
		-n "HC6341-Linux-3.8.3" -d vmlinux-hc6341.bin.lzma uImage
	@raw_size=`stat --format=%s uImage`; \
		pad_size=`expr 1310720 - $$raw_size`; \
		dd if=/dev/zero of=zeros.tmp bs=$$pad_size count=1; \
		cat uImage zeros.tmp > uImage.1280k; \
		rm -f zeros.tmp; \
		cat prefix-1-2.bin uImage.1280k $(root_squashfs) > openwrt-hc6341-recovery.bin;
	@ln -sf openwrt-hc6341-recovery.bin recovery.bin
	@[ -d $(openwrt_root)/bin/ar71xx ] && cp -vf openwrt-hc6341-recovery.bin $(openwrt_root)/bin/ar71xx/ || :

clean:
	@rm -vf vmlinux-hc6341* $(root_squashfs) uImage* openwrt-hc6341-recovery.bin recovery.bin
