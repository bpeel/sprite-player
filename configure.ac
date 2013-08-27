AC_PREREQ(2.59)

AC_INIT(sprite-player, [0.1])
AC_CONFIG_SRCDIR(src/sprite-player.c)
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_MACRO_DIR([build/autotools])
AC_CONFIG_HEADERS(config.h)
AC_GNU_SOURCE

AM_INIT_AUTOMAKE([1.11 foreign -Wno-portability no-define no-dist-gzip
                  dist-bzip2 tar-ustar])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_ISC_POSIX
AC_C_CONST

AC_CHECK_LIB([m], sinf)

ALL_WARNING_CFLAGS="-Wall -Wcast-align -Wuninitialized
                    -Wno-strict-aliasing -Wempty-body -Wformat
                    -Wformat-security -Winit-self -Wundef
                    -Wdeclaration-after-statement -Wvla
                    -Wpointer-arith -Wmissing-declarations"
AS_COMPILER_FLAGS([WARNING_CFLAGS],
                  [$ALL_WARNING_CFLAGS])
AC_SUBST([WARNING_CFLAGS])

PKG_CHECK_MODULES([COGL], [cogl2])
PKG_CHECK_MODULES([COGL_GST], [cogl-gst])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.32 gio-2.0 gobject-2.0])
PKG_CHECK_MODULES([SDL], [sdl2])

AC_OUTPUT(
Makefile
src/Makefile
)
