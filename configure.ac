AC_PREREQ(2.59)

AC_INIT(full-text-search, 0.0.1, haya@tkl.iis.u-tokyo.ac.jp)
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADER([src/config.h])

AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

AC_PROG_LIBTOOL

AC_CHECK_CUTTER

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])

dnl ******************************
dnl Check for GLib
dnl ******************************

GLIB_REQUIRED=2.12.3
AC_SUBST(GLIB_REQUIRED)
AM_PATH_GLIB_2_0([],
                 [],
                 [AC_MSG_ERROR([GLib >= $GLIB_REQUIRED required.])],
                 [gmodule gobject gthread gio])

dnl ******************************
dnl Check for MeCab
dnl ******************************

AC_CHECK_HEADERS(mecab.h)
AC_CHECK_LIB(mecab, mecab_version)

AC_OUTPUT
