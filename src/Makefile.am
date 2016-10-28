bin_PROGRAMS = systemui
systemui_SOURCES = systemui.c dbus.c ipm.c plugin.c

systemui_CFLAGS = \
		$(HILDON_CFLAGS) $(CONNUI_CFLAGS) $(OSSO_CFLAGS) \
		$(GCONF_CFLAGS) $(DBUS_CFLAGS) $(X11_CFLAGS) \
		$(OSSO_SYSTEMUI_DBUS_CFLAGS) $(DBUS_GLIB_CFLAGS)

systemui_LDADD = \
		$(HILDON_LIBS) $(CONNUI_LIBS) $(OSSO_LIBS) \
		$(GCONF_LIBS) $(DBUS_LIBS) $(X11_LIBS) \
		$(OSSO_SYSTEMUI_DBUS_LIBS) $(DBUS_GLIB_LIBS)

systemuiincludedir = $(includedir)/systemui
systemuiinclude_HEADERS = systemui.h

AM_CPPFLAGS = -DLOCALEDIR='"$(localedir)"'
