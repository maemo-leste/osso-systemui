#include <locale.h>
#include <libintl.h>
#include <canberra.h>
#include <mce/dbus-names.h>
#include <hildon/hildon-banner.h>
#include <systemui/dbus-names.h>
#include <osso-log.h>
#include <systemui.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus.h"

/* Those are supposed to be in some osso-locale.h file, can't find it */
#define LOCALE_CHANGED_INTERFACE "com.nokia.LocaleChangeNotification"
#define LOCALE_CHANGED_PATH "/"
#define LOCALE_CHANGED_SIG_NAME "locale_changed"

DBusConnection *session_bus = NULL;
const gchar *vibrator_pattern = "PatternIncomingMessage";
gboolean thermal_shutdown_started = FALSE;

gboolean
dbus_send_message(DBusConnection *dbus, DBusMessage *msg)
{
  g_return_val_if_fail(msg != NULL, FALSE);

  if (!dbus)
    goto err;

  dbus_message_set_no_reply(msg, TRUE);

  if (!dbus_connection_send(dbus, msg, NULL))
  {
    SYSTEMUI_CRITICAL("Failed to send dbus message");
    goto err;
  }

  dbus_message_unref(msg);

  return TRUE;

err:
  dbus_message_unref(msg);

  return FALSE;
}

gboolean vibrator_deactivate(system_ui_data *ui)
{
  DBusMessage *msg;

  msg = dbus_message_new_method_call(MCE_SERVICE,
                                     MCE_REQUEST_PATH,
                                     MCE_REQUEST_IF,
                                     MCE_DEACTIVATE_VIBRATOR_PATTERN);
  dbus_message_append_args(msg,
                           DBUS_TYPE_STRING, &vibrator_pattern,
                           DBUS_TYPE_INVALID);
  dbus_send_message(ui->system_bus, msg);

  return FALSE;
}

void
handle_thermal_notification(system_ui_data *ui, const char *state)
{
  if (!thermal_shutdown_started && state && !strcmp(state, "fatal") )
  {
    DBusMessage *msg;
    ca_context *c = 0;
    int ca_error;
    GtkWidget *banner;

    dbus_send_message(ui->system_bus,
                      dbus_message_new_method_call(MCE_SERVICE,
                                                   MCE_REQUEST_PATH,
                                                   MCE_REQUEST_IF,
                                                   MCE_DISPLAY_ON_REQ));

    msg = dbus_message_new_method_call(MCE_SERVICE,
                                       MCE_REQUEST_PATH,
                                       MCE_REQUEST_IF,
                                       MCE_ACTIVATE_VIBRATOR_PATTERN);
    dbus_message_append_args(msg,
                             DBUS_TYPE_STRING, &vibrator_pattern,
                             DBUS_TYPE_INVALID);
    dbus_send_message(ui->system_bus, msg);

    g_timeout_add_seconds(2, (GSourceFunc)vibrator_deactivate, ui);

    ca_error = ca_context_create(&c);
    if (!ca_error)
    {
      ca_error = ca_context_play(c, 0,
                                 CA_PROP_MEDIA_FILENAME,
                                 "/usr/share/sounds/ui-information_note.wav",
                                 CA_PROP_MEDIA_NAME,
                                 "Thermal Shutdown Notification",
                                 NULL);
    }

    if (ca_error)
      SYSTEMUI_ERROR("Failed to play sound (%d, %s)", ca_error,
                     ca_strerror(ca_error));

    banner =
        hildon_banner_show_information(NULL, NULL,
                                       dgettext("osso-powerup-shutdown",
                                                "dpup_ia_thermal_shutdown"));
    hildon_banner_set_timeout(HILDON_BANNER(banner), 9000);
    gtk_widget_show_all(banner);
    WindowPriority_ShowWindow(banner, 300u);
    thermal_shutdown_started = TRUE;
  }
}

static DBusHandlerResult
dbus_req_handler(DBusConnection *connection, DBusMessage *msg, void *user_data)
{
  system_ui_data *ui = (system_ui_data *)user_data;
  const gchar *dest;
  const gchar *sender;
  int msg_type;
  GArray *args;
  system_ui_handler handler;
  int type;
  dbus_bool_t noreply;
  const gchar *iface;
  const gchar *method;
  DBusMessageIter iter;
  system_ui_handler_arg value;
  int i;

  dest = dbus_message_get_destination(msg);
  iface = dbus_message_get_interface(msg);
  method = dbus_message_get_member(msg);
  sender = dbus_message_get_sender(msg);
  msg_type = dbus_message_get_type(msg);

  if (!msg_type || !sender || !iface || !method)
    goto EXIT;

  if (msg_type == DBUS_MESSAGE_TYPE_METHOD_CALL)
  {
    if (!strcmp(dest, ui->bus_name))
    {
      ULOG_INFO("Method call received from: %s, iface: %s, method: %s", sender,
                iface, method);

      noreply = dbus_message_get_no_reply(msg);
      args = g_array_new(0, 0, sizeof(system_ui_handler_arg));

      if (dbus_message_iter_init(msg, &iter))
      {
        while (1)
        {
          system_ui_handler_arg arg;

          arg.arg_type = dbus_message_iter_get_arg_type(&iter);
          dbus_message_iter_get_basic(&iter, &arg.data);

          if (arg.arg_type == DBUS_TYPE_STRING)
            arg.data.str = g_strdup(arg.data.str);

          g_array_append_vals(args, &arg, 1);

          if (!dbus_message_iter_has_next(&iter))
            break;

          dbus_message_iter_next(&iter);
        }
      }

      if (g_ascii_strcasecmp(iface, ui->requestinterface))
      {
        SYSTEMUI_INFO("Invalid interface");
        type = 'm';
      }
      else
      {
        handler = (system_ui_handler)g_tree_lookup(ui->handlers, method);
        if (handler)
          type = handler(iface, method, args, ui, &value);
        else
        {
          SYSTEMUI_INFO("Unknown method call message");
          type = 'm';
        }
      }

      for (i = 0; i < args->len; i++)
      {
        system_ui_handler_arg *arg = &((system_ui_handler_arg*)args->data)[i];

        if (arg->arg_type == DBUS_TYPE_STRING)
          g_free(arg->data.str);
      }

      g_array_free(args, TRUE);

      if (!noreply)
      {
        DBusMessage *reply;

        if (type)
        {
          if (type == 'm')
          {
            reply = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD,
                                           "No such method");
          }
          else
            reply = dbus_message_new_method_return(msg);
        }
        else
        {
          reply = dbus_message_new_error(msg, DBUS_ERROR_INVALID_ARGS,
                                         DBUS_ERROR_INVALID_ARGS);
        }

        if (reply)
        {
          dbus_message_iter_init_append(reply, &iter);

          if (type == DBUS_TYPE_VARIANT)
          {
            i = 0;
            dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &i);
          }
          else if (type && type != 'm')
            dbus_message_iter_append_basic(&iter, type, &value.data);

          dbus_send_message(connection, reply);
        }
      }
    }
  }
  else
  {
    if (msg_type == DBUS_MESSAGE_TYPE_SIGNAL)
    {

      if (!strcmp(iface, LOCALE_CHANGED_INTERFACE) &&
          !strcmp(method, LOCALE_CHANGED_SIG_NAME))
      {
        gchar *locale = NULL;

        dbus_message_get_args(msg, NULL,
                              DBUS_TYPE_STRING, &locale,
                              DBUS_TYPE_INVALID);
        ULOG_INFO("New locale: %s", locale);

        if (locale)
          setlocale(LC_MESSAGES, locale);

      }
      else if(!strcmp(iface, "com.nokia.thermalmanager") &&
              !strcmp(method, "thermal_state_change_ind"))
      {
          gchar *state = 0;

          dbus_message_get_args(msg, NULL,
                                DBUS_TYPE_STRING, &state,
                                DBUS_TYPE_INVALID);
          handle_thermal_notification(ui, state);
      }
      else if(!strcmp(iface, "com.nokia.dsme.signal"))
      {
        if(!strcmp(method, "denied_req_ind"))
        {
          gchar *action = NULL;
          gchar *reason = NULL;
          gchar *ok_msg = "";
          guint32 style = 0;
          char *message;

          dbus_message_get_args(msg, NULL,
                                DBUS_TYPE_STRING, &action,
                                DBUS_TYPE_STRING, &reason,
                                DBUS_TYPE_INVALID);

          ULOG_INFO("Got DSME denied_req_ind signal, action='%s', reason='%s'",
                    action, reason);

          message = dgettext("osso-powerup-shutdown",
                             "powerup_in_do_not_switch_off");

          msg = dbus_message_new_method_call("org.freedesktop.Notifications",
                                             "/org/freedesktop/Notifications",
                                             "org.freedesktop.Notifications",
                                             "SystemNoteDialog");

          if (msg)
          {
            if (dbus_message_append_args(msg,
                                         DBUS_TYPE_STRING, &message,
                                         DBUS_TYPE_UINT32, &style,
                                         DBUS_TYPE_STRING, &ok_msg,
                                         DBUS_TYPE_INVALID))
            {
              dbus_send_message(session_bus, msg);
            }
            else
              SYSTEMUI_ERROR("Unable to add parameters to dbus call");
          }
          else
            SYSTEMUI_CRITICAL("Unable to create dbus message to SystemNoteDialog");
        }
        else if(!strcmp(method, "shutdown_ind"))
        {
                ULOG_INFO("systemui: shutdown_ind from DSME, quitting");
                gtk_main_quit();
        }
      }
    }
  }

EXIT:
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int
quit_handler(const char *interface, const char *method, GArray *args,
                 system_ui_data *ui, system_ui_handler_arg *result)
{
  gtk_main_quit();

  return DBUS_TYPE_VARIANT;
}

gboolean
dbus_init(system_ui_data *ui)
{
  ui->mainloop = g_main_loop_new(0, 0);
  dbus_error_init(&ui->dbuserror);
  ui->system_bus = dbus_bus_get(DBUS_BUS_SYSTEM, &ui->dbuserror);

  if (!ui->system_bus)
  {
    SYSTEMUI_ERROR("Failed to open connection to system bus");
    dbus_error_free(&ui->dbuserror);
    return FALSE;
  }

  session_bus = dbus_bus_get(DBUS_BUS_SESSION, &ui->dbuserror);

  if (!session_bus )
  {
    int retry = 3;
    DBusError error;

    SYSTEMUI_WARNING("Failed to open connection to session bus, retry");

    dbus_error_free(&ui->dbuserror);
    dbus_error_init(&error);

    while (!session_bus)
    {
      usleep(300 * 1000);
      session_bus = dbus_bus_get(DBUS_BUS_SESSION, &error);

      if (!session_bus)
      {
        SYSTEMUI_WARNING("Failed to connect to session bus: %s, %s, retry",
                         error.name, error.message);
        dbus_error_free(&error);

        if (!--retry)
        {
          SYSTEMUI_ERROR("Failed to open connection to session bus, give up");
          return FALSE;
        }
      }
    }
  }

  if (!dbus_connection_add_filter(ui->system_bus, dbus_req_handler, ui, 0) ||
      !dbus_connection_add_filter(session_bus, dbus_req_handler, ui, 0))
  {
      SYSTEMUI_ERROR("Failed to add dbus filter");
      /* dbus_error_free(&ui->dbuserror); - not needed here */
      return FALSE;
  }

  dbus_bus_add_match(ui->system_bus,
                     "type='signal',interface='com.nokia.LocaleChangeNotification',path='/org/freedesktop/DBus',member='locale_changed'",
                     &ui->dbuserror);
  if (dbus_error_is_set(&ui->dbuserror))
  {
      SYSTEMUI_WARNING("Unable to add match for locale change signal");
      dbus_error_free(&ui->dbuserror);
  }

  dbus_bus_add_match(ui->system_bus,
                     "type='signal',interface='com.nokia.dsme.signal',path='/com/nokia/dsme/signal',member='denied_req_ind'",
                     &ui->dbuserror);
  if (dbus_error_is_set(&ui->dbuserror))
  {
      SYSTEMUI_WARNING("Unable to add match for dsme denied_req_ind signal");
      dbus_error_free(&ui->dbuserror);
  }

  dbus_bus_add_match(ui->system_bus,
                     "type='signal',interface='com.nokia.dsme.signal',path='/com/nokia/dsme/signal',member='shutdown_ind'",
                     &ui->dbuserror);
  if (dbus_error_is_set(&ui->dbuserror))
  {
      SYSTEMUI_WARNING("Unable to add match for dsme shutdown_ind signal");
      dbus_error_free(&ui->dbuserror);
  }

  dbus_connection_setup_with_g_main(ui->system_bus, NULL);
  dbus_connection_setup_with_g_main(session_bus, NULL);
  dbus_connection_set_exit_on_disconnect(session_bus, TRUE);

  if (dbus_bus_request_name(ui->system_bus, ui->bus_name,
                            DBUS_NAME_FLAG_REPLACE_EXISTING, &ui->dbuserror) ==
      DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
      ui->handlers = NULL;
      systemui_add_handler(SYSTEMUI_QUIT_REQ, quit_handler, ui);
      return TRUE;
  }

  SYSTEMUI_ERROR("Failed to acquire dbus service name");
  dbus_error_free(&ui->dbuserror);

  return FALSE;
}

gboolean
dbus_finish(system_ui_data *ui)
{
  DBusError *error = &ui->dbuserror;

  systemui_remove_handler(SYSTEMUI_QUIT_REQ, ui);

  dbus_bus_remove_match(ui->system_bus,
                        "type='signal',interface='com.nokia.LocaleChangeNotification',path='/org/freedesktop/DBus',member='locale_changed'",
                        error);
  if (dbus_error_is_set(error))
  {
    SYSTEMUI_WARNING("Failed to remove match for locale change signal");
    dbus_error_free(error);
  }

  dbus_bus_remove_match(ui->system_bus,
                        "type='signal',interface='com.nokia.dsme.signal',path='/com/nokia/dsme/signal',member='denied_req_ind'",
                        error);
  if (dbus_error_is_set(error))
  {
    SYSTEMUI_WARNING("Failed to remove match for dsme denied_req_ind signal");
    dbus_error_free(error);
  }

  dbus_bus_remove_match(ui->system_bus,
                        "type='signal',interface='com.nokia.dsme.signal',path='/com/nokia/dsme/signal',member='shutdown_ind'",
                        error);
  if (dbus_error_is_set(error))
  {
    SYSTEMUI_WARNING("Failed to remove match for dsme shutdown_ind signal");
    dbus_error_free(error);
  }

  g_tree_destroy(ui->handlers);
  ui->handlers = NULL;

  dbus_connection_unref(ui->system_bus);
  ui->system_bus = NULL;

  dbus_connection_unref(session_bus);
  session_bus = NULL;

  g_main_loop_unref(ui->mainloop);
  ui->mainloop = NULL;

  return TRUE;
}

gboolean
init_thermal_message_rcvr(system_ui_data *ui)
{
  dbus_bus_add_match(ui->system_bus,
                     "type='signal',interface='com.nokia.thermalmanager',member='thermal_state_change_ind'",
                     &ui->dbuserror);

  if (dbus_error_is_set(&ui->dbuserror))
  {
    ULOG_WARN("Failed to add match for thermal notifications");
    dbus_error_free(&ui->dbuserror);
    return FALSE;
  }

  return TRUE;
}
