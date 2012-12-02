#ifndef __SYSTEMUI_H_INCLUDED__
#define __SYSTEMUI_H_INCLUDED__

#include <dbus/dbus.h>
#include <gconf/gconf-client.h>

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
	DBusConnection *session_bus;
	GtkIconTheme *icontheme;
	int unk1;
	GHashTable *hashtable;
	DBusConnection *system_bus;
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
extern guint nsv_sv_play_event(void*, guint type);
extern void nsv_sv_stop_event(void*, guint event_id);

extern void WindowPriority_HideWindow(GtkWidget*);
extern gboolean WindowPriority_ShowWindow(GtkWidget*, int priority);

typedef int (*system_ui_handler)(const char *interface,
                                   const char *method,
                                   GArray *args /* array of system_ui_callback_arg */,
                                   system_ui_data *data,
                                   system_ui_handler_arg *result);

extern int
systemui_check_plugin_arguments(GArray *args, int *supportedargs, guint argc);
extern int
check_plugin_arguments(GArray *args, int *supportedargs, guint argc);

extern int
systemui_check_set_callback(GArray *args, system_ui_callback_t *callback);
extern int
check_set_callback(GArray *args, system_ui_callback_t *callback);

extern void
systemui_do_callback(system_ui_data *data, system_ui_callback_t *callback, guint argc);
extern void
do_callback(system_ui_data *data, system_ui_callback_t *callback, guint argc);

extern void
systemui_free_callback(system_ui_callback_t *callback);

extern void
ipm_hide_window(GtkWidget *widget);
extern void
ipm_show_window(GtkWidget *widget, unsigned int priority);

extern int
systemui_add_handler(const char *name, system_ui_handler handler, system_ui_data *data);
extern void
add_handler(const char *name, system_ui_handler handler, system_ui_data *data);


extern void
systemui_remove_handler(char *name, system_ui_data *data);
extern void
remove_handler(char *name, system_ui_data *data);

void plugin_close(system_ui_data *data);
gboolean plugin_init(system_ui_data *data);

#define SYSTEMUI_TKLOCK_OPEN_REQ       "tklock_open"
#define SYSTEMUI_TKLOCK_CLOSE_REQ      "tklock_close"
#define TKLOCK_SIGNAL_IF		"com.nokia.tklock.signal"
#define TKLOCK_SIGNAL_PATH		"/com/nokia/tklock/signal"
#define TKLOCK_MM_KEY_PRESS_SIG         "mm_key_press"
typedef enum
{
	TKLOCK_MODE_NONE,
	TKLOCK_MODE_ENABLE,
	TKLOCK_MODE_HELP,
	TKLOCK_MODE_SELECT,
	TKLOCK_MODE_ONEINPUT,
	TKLOCK_MODE_ENABLE_VISUAL
} tklock_mode;

typedef enum
{
	TKLOCK_STATUS_NONE,
	TKLOCK_STATUS_UNLOCK,
	TKLOCK_STATUS_RETRY,
	TKLOCK_STATUS_TIMEOUT,
	TKLOCK_STATUS_CLOSED
} tklock_status;


#endif /* __SYSTEMUI_H_INCLUDED__ */
