#include <malloc.h>
#include <osso-log.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <systemui.h>
#include <errno.h>

GSList *plugin_list;

enum plugin_state
{
  UNLOADED,
  LOADED,
  ERROR
};
typedef enum plugin_state plugin_state_t;

typedef gboolean (*plugin_init_f)(system_ui_data *);
typedef void (*plugin_close_f)(system_ui_data *);

struct plugin
{
  gchar *fname;
  plugin_state_t state;
  void *handle;
  plugin_init_f plugin_init;
  plugin_close_f plugin_close;
  system_ui_data *ui;
};
typedef struct plugin plugin_t;

void
plugin_load(plugin_t *plugin, gboolean *previous_ok)
{
  if (!*previous_ok)
  {
    ULOG_WARN("Plugin %s loading skipped, error occured while previous plugin",
              plugin->fname);
    return;
  }

  if (!(plugin->handle = dlopen(plugin->fname, RTLD_NOW)) ||
      !(plugin->plugin_init =
        (plugin_init_f)dlsym(plugin->handle, "plugin_init")) ||
      !(plugin->plugin_close =
        (plugin_close_f)dlsym(plugin->handle, "plugin_close")) ||
      !plugin->plugin_init(plugin->ui))
  {
      goto err;
  }

  plugin->state = LOADED;

  return;

err:
    ULOG_ERR("Failed to load plugin %s (%s)", plugin->fname, dlerror());

    if (plugin->handle)
    {
      dlclose(plugin->handle);
      plugin->handle = 0;
    }

    plugin->state = ERROR;
    *previous_ok = FALSE;
}

void
plugin_unload(plugin_t *plugin, gpointer user_data)
{
  if (plugin->handle)
  {
    plugin->plugin_close(plugin->ui);
    dlclose(plugin->handle);
    plugin->handle = NULL;
  }

  if (plugin->fname)
  {
    free(plugin->fname);
    plugin->fname = NULL;
  }

  plugin->state = UNLOADED;
  free(plugin);
}

void
close_plugins()
{
  ULOG_INFO("Unloading all plugins");
  g_slist_foreach(plugin_list, (GFunc)plugin_unload, NULL);
  g_slist_free(plugin_list);
  plugin_list = NULL;
}

gboolean
init_plugins(system_ui_data *app_ui_data)
{
  gchar *prefix;
  gchar *path;
  gboolean try_load = FALSE;
  gboolean result = FALSE;
  gboolean load_ok = TRUE;

  ULOG_INFO("Loading all plugins");

  prefix = gconf_client_get_string(app_ui_data->gc_client,
                                   SYSTEMUI_GCONF_PLUGIN_PREFIX, NULL);
  if (!prefix)
  {
    ULOG_INFO("GConf key for plugin prefix not found, using default prefix");
    prefix = g_strdup("libsystemuiplugin_");
  }

  path = gconf_client_get_string(app_ui_data->gc_client,
                                 SYSTEMUI_GCONF_PLUGIN_PATH, NULL);
  if (!path)
  {
    ULOG_INFO("GConf key for plugin path not found, using default path");
    path = g_strdup("/usr/lib/systemui/");
  }

  if (prefix && path)
  {
    DIR *dir = opendir(path);
    struct dirent *dirent;
    size_t pefix_len;

    if (!dir)
    {
      ULOG_INFO("plugin directory opendir failed with %s", strerror(errno));
      goto EXIT;
    }

    pefix_len = strlen(prefix);

    for (dirent = readdir(dir); dirent; dirent = readdir(dir))
    {
      if (!telldir(dir))
        break;

      if (!strncmp(dirent->d_name, prefix, pefix_len))
      {
        plugin_t *plugin_list_item = (plugin_t *)malloc(sizeof(plugin_t));

        if (!plugin_list_item)
        {
          SYSTEMUI_ERROR("Failed to allocate memory for plugin_list_item");
          closedir(dir);
          goto EXIT;
        }

        plugin_list_item->handle = NULL;
        plugin_list_item->state = UNLOADED;
        plugin_list_item->ui = app_ui_data;
        plugin_list_item->fname = g_strconcat(path, dirent->d_name, NULL);
        plugin_list = g_slist_append(plugin_list, plugin_list_item);
      }
    }

    closedir(dir);
    try_load = TRUE;
  }

EXIT:
  if (path)
    g_free(path);

  if (prefix)
    g_free(prefix);

  if (try_load)
  {
    g_slist_foreach(plugin_list, (GFunc)plugin_load, &load_ok);

    if (load_ok)
      result = TRUE;
    else
    {
      SYSTEMUI_WARNING("Failed to load (some) plugin(s)");
      close_plugins();
    }
  }
  else
    SYSTEMUI_ERROR("Failed to read plugin names");

  return result;
}
