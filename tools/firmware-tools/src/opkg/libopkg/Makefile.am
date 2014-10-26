HOST_CPU=@host_cpu@
AM_CFLAGS=-Wall -DHOST_CPU_STR=\"$(HOST_CPU)\" -DBUILD_CPU=@build_cpu@ -DLIBDIR=\"@libdir@\" -DOPKGLIBDIR=\"@opkglibdir@\" -DOPKGETCDIR=\"@opkgetcdir@\" -DOPKGLOCKFILE=\"@opkglockfile@\" -DDATADIR=\"@datadir@\" -I$(top_srcdir) $(BIGENDIAN_CFLAGS) $(CURL_CFLAGS) $(GPGME_CFLAGS) $(PATHFINDER_CFLAGS)

libopkg_includedir=$(includedir)/libopkg
libopkg_include_HEADERS= *.h


opkg_libcore_sources = \
		    opkg.c opkg.h \
		    opkg_defines.h
opkg_cmd_sources = opkg_cmd.c opkg_cmd.h \
		   opkg_configure.c opkg_configure.h \
		   opkg_download.c opkg_download.h \
		   opkg_install.c opkg_install.h \
		   opkg_upgrade.c opkg_upgrade.h \
		   opkg_remove.c opkg_remove.h
opkg_db_sources = opkg_conf.c opkg_conf.h \
		  release.c release.h release_parse.c release_parse.h \
		  opkg_utils.c opkg_utils.h pkg.c pkg.h hash_table.h \
		  pkg_depends.c pkg_depends.h pkg_extract.c pkg_extract.h \
		  hash_table.c pkg_hash.c pkg_hash.h pkg_parse.c pkg_parse.h \
		  pkg_vec.c pkg_vec.h
opkg_list_sources = conffile.c conffile.h conffile_list.c conffile_list.h \
		    nv_pair.c nv_pair.h nv_pair_list.c nv_pair_list.h \
		    pkg_dest.c pkg_dest.h pkg_dest_list.c pkg_dest_list.h \
		    pkg_src.c pkg_src.h pkg_src_list.c pkg_src_list.h \
		    str_list.c str_list.h void_list.c void_list.h \
		    active_list.c active_list.h list.h 
opkg_util_sources = file_util.c file_util.h opkg_message.h opkg_message.c md5.c md5.h \
		    parse_util.c parse_util.h \
		    cksum_list.c cksum_list.h \
		    sprintf_alloc.c sprintf_alloc.h \
		    xregex.c xregex.h xsystem.c xsystem.h
if HAVE_PATHFINDER
opkg_util_sources += opkg_pathfinder.c opkg_pathfinder.h
endif
if HAVE_SHA256
opkg_util_sources += sha256.c sha256.h
endif

noinst_LIBRARIES = libopkg.a
libopkg_a_SOURCES = \
	$(opkg_libcore_sources) \
	$(opkg_cmd_sources) $(opkg_db_sources) \
	$(opkg_util_sources) $(opkg_list_sources)

libopkg_a_LIBADD = $(top_builddir)/libbb/libbb.a
