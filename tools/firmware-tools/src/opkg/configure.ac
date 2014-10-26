# Process this file with autoconf to produce a configure script
AC_INIT([opkg], [0.1.8])
AC_CONFIG_SRCDIR([libopkg/pkg.c])

AC_CONFIG_AUX_DIR([conf])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(libopkg/config.h)

AC_CANONICAL_HOST
AC_GNU_SOURCE 

# Disable C++/Fortran checks
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])


for top_builddir in . .. ../.. $ac_auxdir $ac_auxdir/..; do
  test -f $top_builddir/configure && break
done

# large file support can be useful for gpgme
AC_SYS_LARGEFILE


# Checks for programs
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_INSTALL_STRIP
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG([0.20])

# Checks for libraries

dnl extra argument: --enable-pathfinder
AC_ARG_ENABLE(pathfinder,
              AC_HELP_STRING([--enable-pathfinder], [Enable libpathfinder support.
      [[default=no]] ]),
    [want_pathfinder="$enableval"], [want_pathfinder="no"])
dnl Check for libpathfinder
if test "x$want_pathfinder" = "xyes"; then
  PKG_CHECK_MODULES([PATHFINDER], [pathfinder-openssl dbus-1 openssl])
  if test -n "$PATHFINDER_CFLAGS$PATHFINDER_LIBS"; then
    AC_DEFINE(HAVE_PATHFINDER, 1, [we have pathfinder])
  fi
  AC_SUBST(PATHFINDER_CFLAGS)
  AC_SUBST(PATHFINDER_LIBS)
fi
AM_CONDITIONAL(HAVE_PATHFINDER, test "x$want_pathfinder" = "xyes")

# check for libcurl
AC_ARG_ENABLE(curl,
              AC_HELP_STRING([--enable-curl], [Enable downloading with curl
      [[default=yes]] ]),
    [want_curl="$enableval"], [want_curl="yes"])

if test "x$want_curl" = "xyes"; then
  PKG_CHECK_MODULES(CURL, [libcurl])
  AC_DEFINE(HAVE_CURL, 1, [Define if you want CURL support])
fi

# check for sha256
AC_ARG_ENABLE(sha256,
              AC_HELP_STRING([--enable-sha256], [Enable sha256sum check
      (sha256.{c,h} are GPLv3 licensed) [[default=no]] ]),
    [want_sha256="$enableval"], [want_sha256="no"])

if test "x$want_sha256" = "xyes"; then
  AC_DEFINE(HAVE_SHA256, 1, [Define if you want sha256 support])
fi
AM_CONDITIONAL(HAVE_SHA256, test "x$want_sha256" = "xyes")

# check for openssl
AC_ARG_ENABLE(openssl,
              AC_HELP_STRING([--enable-openssl], [Enable signature checking with OpenSSL
      [[default=no]] ]),
    [want_openssl="$enableval"], [want_openssl="no"])

if test "x$want_openssl" = "xyes"; then
  AC_DEFINE(HAVE_OPENSSL, 1, [Define if you want OpenSSL support])
  NEED_SSL_LIBS="yes"
fi

# check for libssl-curl
AC_ARG_ENABLE(ssl-curl,
              AC_HELP_STRING([--enable-ssl-curl], [Enable certificate authentication with curl
                              [[default="yes"]] ]),
                              [want_sslcurl="$enableval"], [want_sslcurl="yes"])

if test "x$want_curl" = "xyes" -a "x$want_sslcurl" = "xyes"; then
  AC_DEFINE(HAVE_CURL, 1, [Define if you want CURL support])
  AC_DEFINE(HAVE_SSLCURL, 1, [Define if you want certificate authentication with curl])
  NEED_SSL_LIBS="yes"
fi

if test "x$NEED_SSL_LIBS" = "xyes"; then
  AC_MSG_CHECKING([if openssl is available])

  PKG_CHECK_MODULES(OPENSSL, openssl, [:], [:])
  if test "x$OPENSSL_LIBS" != "x"; then
          AC_MSG_RESULT(yes)
  else
          OPENSSL_LIBS="-lcrypto -lssl"
          dnl If pkg-config fails, run compile/link test.
          AC_TRY_LINK([
#include <openssl/opensslv.h>
], [
return OPENSSL_VERSION_NUMBER; ],
          [
            AC_MSG_RESULT(yes)

          ], [
            AC_MSG_RESULT(no)
            AC_MSG_ERROR(OpenSSL not found)
          ])
  fi
  AC_SUBST(OPENSSL_LIBS)
fi


dnl **********
dnl GPGME
dnl **********

AC_ARG_ENABLE(gpg,
    AC_HELP_STRING([--enable-gpg], [Enable signature checking with gpgme
      [[default=yes]] ]),
    [want_gpgme="$enableval"], [want_gpgme="yes"])

if test "x$want_gpgme" = "xyes"; then
  ok="no"
  min_gpgme_version=1.0.0
  AC_PATH_PROG(GPGME_CONFIG, gpgme-config, "failed")
  if test $GPGME_CONFIG != "failed" ; then
	AC_MSG_CHECKING(for GPGME - version >= $min_gpgme_version)
	req_major=`echo $min_gpgme_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\1/'`
	req_minor=`echo $min_gpgme_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\2/'`
	req_micro=`echo $min_gpgme_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\)/\3/'`
	gpgme_config_version=`$GPGME_CONFIG --version`
	major=`echo $gpgme_config_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\1/'`
	minor=`echo $gpgme_config_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\2/'`
	micro=`echo $gpgme_config_version | \
		   sed 's/\([[0-9]]*\)\.\([[0-9]]*\)\.\([[0-9]]*\).*/\3/'`

	  if test "$major" -eq "$req_major"; then
	     if test "$minor" -ge "$req_minor"; then
	       if test "$micro" -ge "$req_micro"; then
		 ok="yes"
	       fi
	     fi
	  fi
  fi

  if test $ok = "yes"; then
    GPGME_CFLAGS=`$GPGME_CONFIG --cflags`
    GPGME_LIBS=`$GPGME_CONFIG --libs`
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GPGME, 1, [Define if you want GPG support])
  else
    AC_MSG_ERROR(GPGME $min_gpgme_version or later needed)
  fi
fi

AC_SUBST(GPGME_CFLAGS)
AC_SUBST(GPGME_LIBS)


# Checks for header files
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([errno.h fcntl.h memory.h regex.h stddef.h stdlib.h string.h strings.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks endianness
AC_C_BIGENDIAN(BIGENDIAN_CFLAGS="-DWORDS_BIGENDIAN=1",)
AC_SUBST(BIGENDIAN_CFLAGS)

# Don't do annoying tests that don't work when cross-compiling, just trust them.
# The AC_FUNC_MEMCMP test doesn't work during a cross-compile, disable.
# AC_FUNC_MEMCMP
# The AC_FUNC_STAT test doesn't work during a cross-compile, disable.
# AC_FUNC_STAT

# Checks for library functions
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_TYPE_SIGNAL
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memmove memset mkdir regcomp strchr strcspn strdup strerror strndup strrchr strstr strtol strtoul sysinfo utime])

opkglibdir=
AC_ARG_WITH(opkglibdir,
[  --with-opkglibdir=DIR   specifies directory to put status and info files.
                          "/opkg" is always added so if you want your files 
                          to be in /var/lib/opkg instead of /usr/lib/opkg
                          you should indicate 
                          --with-opkglibdir=/var/lib ],
[case "${withval}" in
yes)    AC_MSG_ERROR(bad value ${withval} given for opkg libs directories ) ;;
no)     ;;
*)      opkglibdir=$with_opkglibdir ;;
esac])

# Default local prefix if it is empty
if test x$opkglibdir = x; then
        opkglibdir=/usr/lib
fi

opkgetcdir=
AC_ARG_WITH(opkgetcdir,
[  --with-opkgetcdir=DIR   specifies directory for opkg.conf file,
                           "/opkg" is always added so if you want your files
                           to be in /usr/etc/opkg instead of /etc/opkg
                           you should indicate
                           --with-opkgetcdir=/usr/etc ],
[case "${withval}" in
yes)    AC_MSG_ERROR(bad value ${withval} given for opkg.conf directory ) ;;
no)     ;;
*)      opkgetcdir=$with_opkgetcdir ;;
esac])

# Default local prefix if it is empty
if test x$opkgetcdir = x; then
        opkgetcdir=/etc
fi

opkglockfile=
AC_ARG_WITH(opkglockfile,
[  --with-opkglockfile=FILE specifies the file used to make sure there is only
                           one instance of opkg runnning.
                           Defaults to ${opkglibdir}/opkg/lock, i.e.
                           /usr/lib/opkg/lock ],
[case "${withval}" in
yes)    AC_MSG_ERROR(bad value ${withval} given for opkg lock file ) ;;
no)     ;;
*)      opkglockfile=$with_opkglockfile ;;
esac])

# Default if empty
if test x$opkglockfile = x; then
        opkglockfile=${opkglibdir}/opkg/lock
fi

dnl Some special cases for the wow64 build
if test "x$want_gpgme" = "xyes"
then
	if test "x$want_openssl" = "xyes"
	then
	AC_MSG_ERROR([--enable-gpg and --enable-openssl are mutually exclusive.
Use --disable-gpg if you want OpenSSL smime signatures])
	fi
fi

CLEAN_DATE=`date +"%B %Y" | tr -d '\n'`

AC_SUBST(opkglibdir)
AC_SUBST(opkgetcdir)
AC_SUBST(opkglockfile)
AC_SUBST([CLEAN_DATE])

AC_OUTPUT(
    Makefile
    libopkg/Makefile
    tests/Makefile
    src/Makefile
    libbb/Makefile
    utils/Makefile
    utils/update-alternatives
    libopkg.pc
    man/Makefile
    man/opkg-cl.1
    man/opkg-key.1
    )
