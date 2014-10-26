AM_CFLAGS = -I${top_srcdir}/libopkg ${ALL_CFLAGS}
bin_PROGRAMS = opkg-cl

opkg_cl_SOURCES = opkg-cl.c
opkg_cl_LDADD = $(top_builddir)/libopkg/libopkg.a \
                $(top_builddir)/libbb/libbb.a \
                $(CURL_LIBS) $(GPGME_LIBS) $(OPENSSL_LIBS) $(PATHFINDER_LIBS)
