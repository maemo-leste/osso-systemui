AC_PREREQ([2.52])

AC_INIT(osso-systemui, 0.2.0.19)

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([HILDON], [hildon-1],
    [AC_DEFINE(WITH_HILDON,[1],[Use Hildon])],
    [PKG_CHECK_MODULES(HILDON, gtk+-3.0, [AC_DEFINE(WITH_GTK3,[1],[Use Gtk3])])])

PKG_CHECK_MODULES(OSSO, libosso)
PKG_CHECK_MODULES(GCONF, gconf-2.0)
PKG_CHECK_MODULES(DBUS, dbus-1)
PKG_CHECK_MODULES(X11, x11)
PKG_CHECK_MODULES(OSSO_SYSTEMUI_DBUS, osso-systemui-dbus)
PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1)
PKG_CHECK_MODULES(CANBERRA, libcanberra)

OLD_CFLAGS=$CFLAGS
CFLAGS="$OLD_CFLAGS $HILDON_CFLAGS -Werror"
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <glib.h>]], [[g_thread_init();]])],
    [AC_DEFINE([HAVE_G_THREAD_INIT],[],["g_thread_init() is deprecated"])], [])
CFLAGS=$OLD_CFLAGS

AC_ARG_ENABLE(cast-checks,  [  --disable-cast-checks   compile with GLIB cast checks disabled],[cchecks=${enableval}],cchecks=yes)
if test "x$cchecks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CAST_CHECKS"
fi

AC_ARG_ENABLE(asserts,      [  --disable-asserts       compile with GLIB assertions disabled],[asserts=${enableval}],asserts=yes)
if test "x$asserts" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_ASSERTS"
fi

AC_ARG_ENABLE(checks,       [  --disable-checks        compile with GLIB checks disabled],[checks=${enableval}],checks=yes)
if test "x$checks" = "xno"; then
    CFLAGS="$CFLAGS -DG_DISABLE_CHECKS"
fi

AC_ARG_ENABLE(debug,     [  --enable-debug          enable printing of debug messages],[ddebug=${enableval}],ddebug=no)
if test "x$ddebug" != "xyes"; then
    CFLAGS="$CFLAGS -DG_DEBUG_DISABLE"
fi

TEXT_DOMAIN=systemui
AC_SUBST(TEXT_DOMAIN)
AC_DEFINE_UNQUOTED(TEXT_DOMAIN, "$TEXT_DOMAIN", [Text domain])

localedir=${datadir}/locale
AC_SUBST(localedir)

AC_OUTPUT
