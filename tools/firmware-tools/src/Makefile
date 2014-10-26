
all: squashfs-tools/mksquashfs squashfs-tools/unsquashfs padjffs2/padjffs2 opkg/src/opkg-cl hexof/hexof

squashfs-tools/mksquashfs squashfs-tools/unsquashfs:
	make -C squashfs-tools
padjffs2/padjffs2:
	make -C padjffs2
hexof/hexof:
	make -C hexof
opkg/src/opkg-cl:
	cd opkg; ./autogen.sh --disable-curl --disable-gpg --with-opkgetcdir=/etc --with-opkglockfile=/tmp/opkg.lock
	make -C opkg

clean:
	make clean -C squashfs-tools
	make clean -C padjffs2
	make clean -C opkg

install: all
	install -m755 squashfs-tools/mksquashfs squashfs-tools/unsquashfs /usr/bin/
	install -m755 padjffs2/padjffs2 /usr/bin/
	install -m755 opkg/src/opkg-cl /usr/bin/opkg
	install -m755 hexof/hexof /usr/bin/
	install -m755 ../merge-firmware.sh /usr/bin/
	mkdir -p /usr/share/firmware-merger
	cp -f ../*-oemparts.bin /usr/share/firmware-merger/

