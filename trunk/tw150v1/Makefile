#
#
root_squashfs = openwrt-ar71xx-generic-root.squashfs
openwrt_root = $(shell pwd)/../openwrt-ar71xx

build_tw150v1: prefix-1-2.bin
	cp $(openwrt_root)/build_dir/target-mips*/linux-ar71xx_generic/vmlinux vmlinux-tw150v1
	$(openwrt_root)/staging_dir/host/bin/patch-cmdline  vmlinux-tw150v1 "board=tw150v1 console=ttyATH0,115200"
	$(openwrt_root)/staging_dir/host/bin/lzma  e vmlinux-tw150v1 -lc1 -lp2 -pb2 vmlinux-tw150v1.bin.lzma
	cp $(openwrt_root)/bin/ar71xx/$(root_squashfs) ./
	mkimage -A mips -O linux -T kernel -a 0x80060000 -C lzma -e 0x80060000 \
		-n "tw150v1-Linux-3.8.3" -d vmlinux-tw150v1.bin.lzma uImage
	@raw_size=`stat --format=%s uImage`; \
		pad_size=`expr 1310720 - $$raw_size`; \
		dd if=/dev/zero of=__pad.tmp bs=$$pad_size count=1; \
		cat uImage __pad.tmp > uImage.1280k; \
		rm -vf __pad.tmp; \
		cat prefix-1-2.bin uImage.1280k $(root_squashfs) > openwrt-tw150v1-revoery.bin;
	@[ -d tftpboot ] && cp -vf openwrt-tw150v1-revoery.bin /tftpboot/recovery.bin || :

prefix-1-2.bin: recovery.bin
# Get first 128K bytes: u-boot, bdinfo
	head recovery.bin -c131072 > prefix-1-2.bin
# Check if it is a valid prefix image or truncated
	@prefix_size=`stat --format=%s prefix-1-2.bin`; \
		if [ $$prefix_size -ne 131072 ]; then \
			echo "*** prefix-1-2.bin has invalid size. Please remove it, 'svn up' and try."; \
			rm -f prefix-1-2.bin; \
			exit 1; \
		fi;

clean:
	rm -vf vmlinux-tw150v1* $(root_squashfs) uImage* openwrt-tw150v1-recovery.bin
