/**
  Copyright (C) 2011 Jonathan Wilson
  Copyright (C) 2012 Ivaylo Dimitrov <freemangordon@abv.bg>

  Contact: Jonathan Wilson <jfwfreo@tpgi.com.au>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#ifndef __SYSTEMUI_H_INCLUDED__
#define __SYSTEMUI_H_INCLUDED__

#include <gtk/gtk.h>
#include <dbus/dbus.h>
#include <gconf/gconf-client.h>
#include <syslog.h>

typedef struct
{
  GTree *handlers;
  char *requestinterface;
  char *signalinterface;
  char *requestpath;
  char *signalpath;
  char *bus_name;
  GConfClient *gc_client;
  DBusError dbuserror;
  GMainLoop *mainloop;
  DBusConnection *system_bus;
  GtkIconTheme *icontheme;
  GHashTable *hsl_tab;
} system_ui_data;

typedef struct
{
  dbus_uint32_t first32;
  dbus_uint32_t second32;
} DBus8ByteStruct;

typedef union
{
  unsigned char bytes[8];
  dbus_int16_t i16;
  dbus_uint16_t u16;
  dbus_int32_t i32;
  dbus_uint32_t u32;
  dbus_bool_t bool_val;
#ifdef DBUS_HAVE_INT64
  dbus_int64_t i64;
  dbus_uint64_t u64;
#endif
  DBus8ByteStruct eight;
  double dbl;
  unsigned char byt;
  char *str;
  int fd;
} DBusBasicValue;

typedef struct
{
  int arg_type;
  DBusBasicValue data;
} system_ui_handler_arg;

typedef struct
{
  char *interface;
  char *path;
  char *destination;
  char *method;
} system_ui_callback_t;

extern void nsv_sv_init(void*);
extern void nsv_sv_shutdown(void*);

/* returns event_id */
extern guint nsv_sv_play_event(void*, guint type, const gchar* sound_file,
                               guint, const gchar* vibra_pattern, guint,
                               guint volume);
extern void nsv_sv_stop_event(void*, guint event_id);

extern gboolean WindowPriority_HideWindow(GtkWidget *);
extern gboolean WindowPriority_ShowWindow(GtkWidget *, unsigned int priority);

typedef int (*system_ui_handler)(const char *interface,
                                   const char *method,
                                   GArray *args /* array of system_ui_callback_arg */,
                                   system_ui_data *ui,
                                   system_ui_handler_arg *result);

extern gboolean
systemui_check_plugin_arguments(GArray *args, int *supportedargs, guint argc);
extern gboolean
check_plugin_arguments(GArray *args, int *supportedargs, guint argc);

extern int
systemui_check_set_callback(GArray *args, system_ui_callback_t *callback);
extern int
check_set_callback(GArray *args, system_ui_callback_t *callback);

extern void
systemui_do_callback(system_ui_data *data, system_ui_callback_t *callback,
                     guint argc);
extern void
do_callback(system_ui_data *ui, system_ui_callback_t *callback, guint argc);

extern void
systemui_free_callback(system_ui_callback_t *callback);

extern gboolean
ipm_hide_window(GtkWidget *widget);
extern gboolean
ipm_show_window(GtkWidget *widget, unsigned int priority);

extern gboolean
systemui_add_handler(const char *name, system_ui_handler handler,
                     system_ui_data *ui);
extern gboolean
add_handler(const char *name, system_ui_handler handler, system_ui_data *ui);


extern gboolean
systemui_remove_handler(const char *name, system_ui_data *ui);
extern gboolean
remove_handler(const char *name, system_ui_data *ui);

void plugin_close(system_ui_data *ui);
gboolean plugin_init(system_ui_data *ui);

#ifdef DEBUG

#define SYSTEMUI_DEBUG(msg, ...) \
  syslog(LOG_MAKEPRI(LOG_USER, LOG_DEBUG), "%s:%d:" msg "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
  #define SYSTEMUI_DEBUG(msg, ...)

#endif

#define SYSTEMUI_DEBUG_FN SYSTEMUI_DEBUG("")

#define SYSTEMUI_CRITICAL(msg, ...) \
  syslog(LOG_MAKEPRI(LOG_USER, LOG_CRIT), "%s:%d:" msg "\n", __func__, __LINE__, ##__VA_ARGS__)

#define SYSTEMUI_ERROR(msg, ...) \
  syslog(LOG_MAKEPRI(LOG_USER, LOG_ERR), "%s:%d:" msg "\n", __func__, __LINE__, ##__VA_ARGS__)

#define SYSTEMUI_WARNING(msg, ...) \
  syslog(LOG_MAKEPRI(LOG_USER, LOG_WARNING), "%s:%d:" msg "\n", __func__, __LINE__, ##__VA_ARGS__)

#define SYSTEMUI_NOTICE(msg, ...) \
  syslog(LOG_MAKEPRI(LOG_USER, LOG_NOTICE), "%s:%d:" msg "\n", __func__, __LINE__, ##__VA_ARGS__)

#define SYSTEMUI_INFO(msg, ...) \
  syslog(LOG_MAKEPRI(LOG_USER, LOG_INFO), "%s:%d:" msg "\n", __func__, __LINE__, ##__VA_ARGS__)

#endif /* __SYSTEMUI_H_INCLUDED__ */
