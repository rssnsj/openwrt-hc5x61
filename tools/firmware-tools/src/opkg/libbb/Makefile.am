HOST_CPU=@host_cpu@
BUILD_CPU=@build_cpu@
ALL_CFLAGS=-g -O -Wall -DHOST_CPU_STR=\"$(HOST_CPU)\" -DBUILD_CPU=@build_cpu@

noinst_LIBRARIES = libbb.a

libbb_a_SOURCES = gz_open.c \
	libbb.h \
	unzip.c \
	wfopen.c \
	unarchive.c \
	copy_file.c \
	copy_file_chunk.c \
	xreadlink.c \
	concat_path_file.c \
	xfuncs.c \
	last_char_is.c \
	make_directory.c \
	safe_strncpy.c \
	parse_mode.c \
	time_string.c \
	all_read.c \
	mode_string.c

libbb_la_CFLAGS = $(ALL_CFLAGS)
#libbb_la_LDFLAGS = -static
