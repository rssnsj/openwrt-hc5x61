
all: src/squashfs-tools/mksquashfs src/squashfs-tools/unsquashfs \
	src/padjffs2/padjffs2 src/opkg/src/opkg-cl src/hexof/hexof

src/squashfs-tools/mksquashfs src/squashfs-tools/unsquashfs:
	make -C src/squashfs-tools
src/padjffs2/padjffs2:
	make -C src/padjffs2
src/opkg/src/opkg-cl:
	cd src/opkg; ./autogen.sh --disable-curl --disable-gpg --with-opkgetcdir=/etc --with-opkglockfile=/tmp/opkg.lock
	make -C src/opkg
src/hexof/hexof:
	make -C src/hexof

clean:
	make clean -C src/squashfs-tools
	make clean -C src/padjffs2
	make clean -C src/opkg
	make clean -C src/hexof

install: all
	install -m755 src/squashfs-tools/mksquashfs src/squashfs-tools/unsquashfs /usr/bin/
	install -m755 src/padjffs2/padjffs2 /usr/bin/
	install -m755 src/opkg/src/opkg-cl /usr/bin/opkg
	install -m755 src/hexof/hexof /usr/bin/
	install -m755 openwrt-repack.sh /usr/bin/

