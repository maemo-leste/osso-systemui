#ifndef SYSTEMUI_DBUS_H
#define SYSTEMUI_DBUS_H

gboolean dbus_send_message(DBusConnection *dbus, DBusMessage *msg);
gboolean init_thermal_message_rcvr(system_ui_data *app_ui_data);
gboolean dbus_init(system_ui_data *ui);
gboolean dbus_finish(system_ui_data *ui);

#endif // SYSTEMUI_DBUS_H
