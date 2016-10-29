#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <systemui.h>

struct window_priority
{
  guint32 priority;
  GtkWidget *widget;
};
typedef struct window_priority window_priority_t;

extern system_ui_data *app_ui_data;

GSList *window_priority_list = NULL;
guint window_prio_max = 300;

static gint
window_priority_compare(window_priority_t *a, window_priority_t *b)
{
  return a->priority != b->priority && a->widget != b->widget;
}

static gint
window_priority_compare_priority(window_priority_t *a, window_priority_t *b)
{
  gint rv;

  if (a->priority >= b->priority)
    rv = a->priority != b->priority;
  else
    rv = -1;

  return rv;
}

gboolean
ipm_show_window(GtkWidget *widget, unsigned int priority)
{
  window_priority_t *wp;
  window_priority_t data;

  if (!widget || priority > window_prio_max)
    return FALSE;

  wp = (window_priority_t *)g_malloc(sizeof(window_priority_t));
  if (!wp)
    return FALSE;

  data.priority = priority;
  data.widget = widget;

  if (!g_slist_find_custom(window_priority_list, &data,
                           (GCompareFunc)window_priority_compare))
  {
    GdkDisplay *dpy = gdk_display_get_default();
    gpointer layer;

    wp->priority = priority;
    wp->widget = widget;
    gtk_widget_realize(widget);

    g_assert(app_ui_data->hsl_tab != NULL);

    layer = g_hash_table_lookup(app_ui_data->hsl_tab, &priority);
    if (layer)
    {
      Atom hsl_atom =
          gdk_x11_get_xatom_by_name_for_display(dpy,
                                                "_HILDON_STACKING_LAYER");

      XChangeProperty(gdk_x11_display_get_xdisplay(dpy),
                      gdk_x11_drawable_get_xid(widget->window), hsl_atom,
                      XA_CARDINAL, 32, PropModeReplace,
                      (unsigned char *)layer, 1);
    }

    window_priority_list = g_slist_insert_sorted(window_priority_list, wp,
                             (GCompareFunc)window_priority_compare_priority);
    gtk_widget_show_all(widget);

    return TRUE;
  }

  return FALSE;
}

gboolean
ipm_hide_window(GtkWidget *widget)
{
  GSList *l;
  window_priority_t tmp;

  if (!widget || !window_priority_list || !g_slist_length(window_priority_list))
    return FALSE;

  tmp.priority = window_prio_max;
  tmp.widget = widget;

  if (!(l = g_slist_find_custom(window_priority_list, &tmp,
                          (GCompareFunc)window_priority_compare)))
    return FALSE;

  if (l->data)
    g_free(l->data);

  window_priority_list = g_slist_delete_link(window_priority_list, l);
  gtk_widget_hide_all(widget);

#if 0 /* Why is that? */
  g_slist_length(window_priority_list);
#endif

  return TRUE;
}
