#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <getopt.h>
#include <osso-log.h>
#include <systemui.h>
#include <libintl.h>
#include <systemui/dbus-names.h>
#include <errno.h>

#include "dbus.h"
#include "plugin.h"

#include "config.h"

struct hsl_prio_map
{
  const char *name;
  int prio;
  int layer;
};

struct hsl_prio_map prios_map[] =
{
  {"ThermalShutdownNote", 300, 9},
  {"TouchScreenLock", 290, 9},
  {"AlarmDialog", 255, 7},
  {"EmergencyCallDialog", 254, 7},
  {"NokiaLogoSplash", 200, 10},
  {"PowerKeyMenu", 195, 10},
  {"SwitchOffNote", 122, 3},
  {"DeviceLock", 121, 3},
  {"DeviceLockBg", 120, 2},
  {"ModeChangeDialog", 60, 1},
  {"ActingDeadScreen", 1, 1}
};

system_ui_data *app_ui_data = NULL;
guint32 uint32arg = 'u';

void
systemui_do_callback(system_ui_data *ui, system_ui_callback_t *callback,
                     guint argc)
{
  DBusMessage *msg;

  if (!callback || !callback->destination || !callback->path ||
      !callback->interface || !callback->method)
  {
    SYSTEMUI_CRITICAL("Invalid callback");
    return;
  }

  msg = dbus_message_new_method_call(callback->destination, callback->path,
                                     callback->interface, callback->method);
  if (!msg)
  {
    SYSTEMUI_CRITICAL("Failed to create callback message");
    return;
  }

  dbus_message_set_no_reply(msg, TRUE);
  if (!dbus_message_append_args(msg,
                                DBUS_TYPE_INT32, &argc,
                                DBUS_TYPE_INVALID))
  {
    SYSTEMUI_CRITICAL("Failed to append parameter to callback");
    dbus_message_unref(msg);
  }
  else
    dbus_send_message(ui->system_bus, msg);
}

void
do_callback(system_ui_data *ui, system_ui_callback_t *callback, guint argc)
{
  systemui_do_callback(ui, callback, argc);
}

void
systemui_free_callback(system_ui_callback_t *callback)
{
  if (callback->destination)
  {
    free(callback->destination);
    callback->destination = NULL;
  }

  if (callback->interface)
  {
    free(callback->interface);
    callback->interface = NULL;
  }

  if (callback->path)
  {
    free(callback->path);
    callback->path = NULL;
  }

  if (callback->method)
  {
    free(callback->method);
    callback->method = NULL;
  }
}

void
free_callback(system_ui_callback_t *callback)
{
  systemui_free_callback(callback);
}

gboolean
systemui_remove_handler(const char *name, system_ui_data *ui)
{
  gconstpointer handler;

  g_return_val_if_fail(ui->handlers != NULL, FALSE);

  if (!(handler = g_tree_lookup(ui->handlers, name)))
    return FALSE;

  g_tree_remove(ui->handlers, handler);

  return TRUE;
}

gboolean
remove_handler(const char *name, system_ui_data *ui)
{
  return systemui_remove_handler(name, ui);
}

gboolean
systemui_add_handler(const char *name, system_ui_handler handler,
                     system_ui_data *ui)
{
  if (!ui->handlers)
    ui->handlers = g_tree_new((GCompareFunc)g_ascii_strcasecmp);

  if (ui->handlers)
  {
    if (!g_tree_lookup(ui->handlers, name))
    {
      g_tree_insert(ui->handlers, strdup(name), handler);
      return TRUE;
    }
  }
  else
    SYSTEMUI_ERROR("failed to allocate memory for handlers tree");

  return FALSE;
}

gboolean
add_handler(const char *name, system_ui_handler handler, system_ui_data *ui)
{
  return systemui_add_handler(name, handler, ui);
}

gboolean
systemui_check_plugin_arguments(GArray *args, int *supportedargs, guint argc)
{
  system_ui_handler_arg *ui_args;
  int i;

  if (args->len != argc + 4)
    return FALSE;

  ui_args = (system_ui_handler_arg *)args->data;

  for (i = 0; i < 4; i++)
  {
    if (ui_args->arg_type != DBUS_TYPE_STRING)
    {
      ULOG_INFO("No callback specified");
      return FALSE;
    }

    ui_args++;
  }

  for (i = 0; i < argc; i++)
  {
    if (ui_args->arg_type != supportedargs[i])
    {
      SYSTEMUI_WARNING("Extra argument %i, of type %i, supported type %i", i,
                       ui_args->arg_type, supportedargs[i]);
      return FALSE;
    }

    ui_args++;
  }

  return TRUE;
}

gboolean
check_plugin_arguments(GArray *args, int *supportedargs, guint argc)
{
  return systemui_check_plugin_arguments(args, supportedargs, argc);
}

gboolean
WindowPriority_ShowWindow(GtkWidget *widget, unsigned int priority)
{
  return ipm_show_window(widget, priority);
}

gboolean
WindowPriority_HideWindow(GtkWidget *widget)
{
  return ipm_hide_window(widget);
}

gboolean
systemui_check_callback(GArray *args, system_ui_callback_t *callback)
{
  system_ui_handler_arg *ui_args = (system_ui_handler_arg *)args->data;

  if (!callback->interface && !callback->path && !callback->destination &&
      !callback->method )
  {
    return TRUE;
  }

  if (g_ascii_strcasecmp(ui_args[0].data.str, callback->destination) ||
      g_ascii_strcasecmp(ui_args[1].data.str, callback->path) ||
      g_ascii_strcasecmp(ui_args[2].data.str, callback->interface) ||
      g_ascii_strcasecmp(ui_args[3].data.str, callback->method))
  {
    return FALSE;
  }

  return TRUE;
}

gboolean
check_callback(GArray *args, system_ui_callback_t *callback)
{
  return systemui_check_callback(args, callback);
}

gboolean
systemui_check_set_callback(GArray *args, system_ui_callback_t *callback)
{
  system_ui_handler_arg *ui_args = (system_ui_handler_arg *)args->data;

  if (!systemui_check_callback(args, callback))
    return FALSE;

  if (!callback->interface)
  {
    callback->destination = g_strdup(ui_args[0].data.str);
    callback->path = g_strdup(ui_args[1].data.str);
    callback->interface = g_strdup(ui_args[2].data.str);
    callback->method = g_strdup(ui_args[3].data.str);
  }

  return TRUE;
}

gboolean
check_set_callback(GArray *args, system_ui_callback_t *callback)
{
  return systemui_check_set_callback(args, callback);
}

void daemonize()
{
  __pid_t pid = fork();
  int fd;
  char buf[2 * sizeof(__pid_t)]; /*should be large enough */

  if (pid == -1)
    ULOG_CRIT("daemonize: fork failed: %s", strerror(errno));

  if (pid)
    exit(0);

  if (setsid() < 0)
    ULOG_WARN("setsid failed: %s", strerror(errno));

  close(STDIN_FILENO);
  close(STDOUT_FILENO);

  fd = open("/dev/null", O_RDWR);
  dup(fd);
  dup(fd);

  chdir("/tmp");

  fd = open("/var/run/systemui.pid", O_CREAT | O_RDWR, 0640);

  if (fd < 0)
  {
    ULOG_ERR("Cannot open lockfile. Exiting.");
    exit(1);
  }

  if (lockf(fd, F_TLOCK, 0) < 0)
  {
    ULOG_ERR("Already running. Exiting.");
    exit(1);
  }

  sprintf(buf, "%d\n", getpid());
  write(fd, buf, strlen(buf));

  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
}

void build_layers_tab()
{
  GHashTable *hsl_tab = g_hash_table_new(g_int_hash, g_int_equal);
  int i;

  if (!hsl_tab )
  {
    SYSTEMUI_ERROR("g_hash_table_new failed");
    g_free(app_ui_data);
    exit(1);
  }

  for (i = 0; i < sizeof(prios_map) / sizeof(prios_map[0]); i++)
    g_hash_table_insert(hsl_tab, &prios_map[i].prio, &prios_map[i].layer);

  app_ui_data->hsl_tab = hsl_tab;
}

void sigterm_handler()
{
  gtk_main_quit();
}

static void usage(const char *program)
{
  fprintf(
    stdout,
    "Usage: %s [OPTION]...\n"
    "System UI\n"
    "\n"
    "  -d, --daemon        run systemui as a daemon\n"
    "      --help          display this help and exit\n"
    "      --version       output version information and exit\n"
    "\n",
    program);
}

void g_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                   const gchar *message, gpointer user_data)
{
}

int
main(int argc, char **argv)
{
  gboolean daemonflag = FALSE;
  int opt;
  int ind;
  static struct option long_options[] =
  {
    {"daemon", 0, 0, 'd'},
    {"help", 0, 0, 'h'},
    {"version", 0, 0, 'V'},
    {0, 0, 0, 0}
  };

  openlog(TEXT_DOMAIN, LOG_NDELAY | LOG_PID, LOG_USER);
  setlocale(LC_ALL, "");

  ULOG_INFO("Starting up as %s %s, locale: %s at %s",
            PACKAGE_NAME,
            PACKAGE_VERSION,
            TEXT_DOMAIN,
            "/usr/share/locale");

  bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
  bind_textdomain_codeset(TEXT_DOMAIN, "UTF-8");
  textdomain(TEXT_DOMAIN);

  while (1)
  {
    opt = getopt_long(argc, argv, "dS", long_options, &ind);

    if (opt == -1)
      break;

    daemonflag = TRUE;

    if (opt != 'd')
    {
      if (opt == 'h' || opt != 'V')
      {
        usage(argv[0]);
        exit(0);
      }

      fprintf(stdout, "%s v%s", PACKAGE_NAME, PACKAGE_VERSION);
      exit(0);
    }
  }

  if (daemonflag)
    daemonize();

  signal(SIGTERM, sigterm_handler);

  g_log_set_default_handler(g_log_handler, 0);
  app_ui_data = (system_ui_data *)g_malloc0(sizeof(system_ui_data));

  if (!app_ui_data)
  {
    SYSTEMUI_CRITICAL("Memory allocation failure");
    exit(1);
  }

  app_ui_data->requestinterface = SYSTEMUI_REQUEST_IF;
  app_ui_data->signalinterface = SYSTEMUI_SIGNAL_IF;
  app_ui_data->requestpath = SYSTEMUI_REQUEST_PATH;
  app_ui_data->signalpath = SYSTEMUI_SIGNAL_PATH;
  app_ui_data->bus_name = SYSTEMUI_SERVICE;
  app_ui_data->handlers = 0;

  build_layers_tab();

  gtk_init(&argc, &argv);
  g_thread_init(0);
  app_ui_data->icontheme = gtk_icon_theme_get_default();
  app_ui_data->gc_client = gconf_client_get_default();

  g_return_val_if_fail(app_ui_data->gc_client, 1);

  gconf_client_add_dir(app_ui_data->gc_client, "/system/systemui/",
                       GCONF_CLIENT_PRELOAD_NONE, NULL);

  g_return_val_if_fail(dbus_init(app_ui_data), 1);
  g_return_val_if_fail(init_thermal_message_rcvr(app_ui_data), 1);

  if (init_plugins(app_ui_data))
  {
    gconf_client_clear_cache(app_ui_data->gc_client);

    if (app_ui_data->system_bus)
    {
      DBusMessage *msg = dbus_message_new_signal(SYSTEMUI_SIGNAL_PATH,
                                                 SYSTEMUI_SIGNAL_IF,
                                                 SYSTEMUI_STARTED_SIG);
      if (msg && dbus_send_message(app_ui_data->system_bus, msg))
      {
        gtk_main();
        ULOG_INFO("Received signal to quit, quitting");
      }
    }

    close_plugins();
  }

  dbus_finish(app_ui_data);
  gconf_client_remove_dir(app_ui_data->gc_client, "/system/systemui/", NULL);
  g_object_unref(app_ui_data->gc_client);

  app_ui_data->gc_client = NULL;
  app_ui_data->system_bus = NULL;
  g_hash_table_unref(app_ui_data->hsl_tab);

  app_ui_data->hsl_tab = NULL;
  g_free(app_ui_data);
  closelog();

  return 0;
}
