
IMAGEBUILDER ?= OpenWrt-ImageBuilder-ramips_mt7620a-for-linux-x86_64

all: .check_deps $(IMAGEBUILDER)/.patched-hc5761
	make -C $(IMAGEBUILDER) image PROFILE=HiWiFi-HC5761 USER_FILES=$(shell cd $(IMAGEBUILDER); pwd)/target/linux/ramips/base-files

$(IMAGEBUILDER)/.patched-hc5761:
	patch -d $(IMAGEBUILDER) -p0 < hiwifi-hc5761.patch
	touch $(IMAGEBUILDER)/.patched-hc5761

.check_deps:
	@if [ ! -d "$(IMAGEBUILDER)" ]; then \
		echo "Please specify a valid ImageBuilder directory by \"make IMAGEBUILDER=...\"."; \
		exit 1; \
	 fi

clean:
	make clean -C $(IMAGEBUILDER)
	@if [ -e $(IMAGEBUILDER)/.patched-hc5761 ]; then \
		rm -vf $(IMAGEBUILDER)/.patched-hc5761; \
		patch -R -d $(IMAGEBUILDER) -p0 < hiwifi-hc5761.patch; \
	fi

