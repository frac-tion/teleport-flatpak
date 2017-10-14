#include <gtk/gtk.h>

#include "teleport-app.h"
#include "teleport-window.h"
#include "teleport-server.h"
#include "teleport-peer.h"

GtkWidget *find_child(GtkWidget *, const gchar *);
TeleportWindow *mainWin;

struct _TeleportWindow
{
  GtkApplicationWindow parent;
};

typedef struct _TeleportWindowPrivate TeleportWindowPrivate;

struct _TeleportWindowPrivate
{
  GSettings *settings;
  GtkWidget *gears;
  GtkWidget *remote_devices_list;
  GtkWidget *this_device_name_label;
  GtkWidget *remote_no_devices;
};

G_DEFINE_TYPE_WITH_PRIVATE(TeleportWindow, teleport_window, GTK_TYPE_APPLICATION_WINDOW);

static void
teleport_window_init (TeleportWindow *win)
{
  TeleportWindowPrivate *priv;
  GtkBuilder *builder;
  GtkWidget *menu;
  GtkEntry *downloadDir;
  mainWin = win;

  priv = teleport_window_get_instance_private (win);
  priv->settings = g_settings_new ("com.frac_tion.teleport");

  if (g_settings_get_user_value (priv->settings, "download-dir") == NULL) {
    g_print ("Download dir set to XDG DOWNLOAD directory\n");
    if (g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD) != NULL) {
      g_settings_set_string (priv->settings,
                             "download-dir",
                             g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD));
    }
    else {
      g_print ("Error: XDG DOWNLOAD is not set.\n");
    }
  }


  if (g_settings_get_user_value (priv->settings, "device-name") == NULL) {
    g_settings_set_string (priv->settings,
                           "device-name",
                           g_get_host_name());
  }

  gtk_widget_init_template (GTK_WIDGET (win));

  builder = gtk_builder_new_from_resource ("/com/frac_tion/teleport/settings.ui");
  menu = GTK_WIDGET (gtk_builder_get_object (builder, "settings"));
  downloadDir = GTK_ENTRY (gtk_builder_get_object (builder, "settings_download_directory"));

  gtk_menu_button_set_popover(GTK_MENU_BUTTON (priv->gears), menu);

  gtk_label_set_text (GTK_LABEL (priv->this_device_name_label),
                      g_settings_get_string (priv->settings, "device-name"));

  g_settings_bind (priv->settings, "download-dir",
                   downloadDir, "text",
                   G_SETTINGS_BIND_DEFAULT);

  //g_object_unref (menu);
  //g_object_unref (label);
  g_object_unref (builder);
}

static void
open_file_picker(GtkButton *btn,
                 Peer *device) {
  GtkWidget *dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  gint res;
  g_print("Open file chooser for submitting a file to %s with Address %s\n", device->name, device->ip);

  dialog =  gtk_file_chooser_dialog_new ("Open File",
                                         GTK_WINDOW(mainWin),
                                         action,
                                         ("_Cancel"),
                                         GTK_RESPONSE_CANCEL,
                                         ("_Open"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    {
      char *filename;
      GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
      filename = gtk_file_chooser_get_filename (chooser);
      g_print("Choosen file is %s\n", filename);
      gtk_widget_destroy (dialog);
      teleport_server_add_route (g_compute_checksum_for_string (G_CHECKSUM_SHA256, filename,  -1), filename, device->ip);
      g_free (filename);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

void
update_remote_device_list(TeleportWindow *win,
                          Peer *device)
{
  TeleportWindowPrivate *priv;
  GtkBuilder *builder_remote_list;
  GtkWidget *row;
  GtkLabel *name_label;
  GtkButton *send_btn;
  //GtkWidget *line;

  priv = teleport_window_get_instance_private (win);

  gtk_widget_hide (priv->remote_no_devices);

  builder_remote_list = gtk_builder_new_from_resource ("/com/frac_tion/teleport/remote_list.ui");

  row = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_device_row"));
  name_label = GTK_LABEL (gtk_builder_get_object (builder_remote_list, "device_name"));
  gtk_label_set_text(name_label, device->name);
  gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), row, -1);
  send_btn = GTK_BUTTON (gtk_builder_get_object (builder_remote_list, "send_btn"));
  g_signal_connect (send_btn, "clicked", G_CALLBACK (open_file_picker), device);

  //line = GTK_WIDGET (gtk_builder_get_object (builder_remote_list, "remote_space_row"));
  //gtk_list_box_insert(GTK_LIST_BOX(priv->remote_devices_list), line, -1);
  g_object_unref (builder_remote_list);
}


void
update_remote_device_list_remove(TeleportWindow *win,
                                 Peer *device)
{
  TeleportWindowPrivate *priv;
  GtkWidget *box;
  GtkListBoxRow *remote_row;
  GtkLabel *name_label;
  gint i = 0;

  priv = teleport_window_get_instance_private (win);
  box = priv->remote_devices_list;

  remote_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), i);

  while (remote_row != NULL) {
    name_label = GTK_LABEL(find_child(GTK_WIDGET(remote_row), "GtkLabel"));
    if (name_label != NULL && g_strcmp0(device->name, gtk_label_get_text(name_label)) == 0) {
      gtk_container_remove (GTK_CONTAINER(box), GTK_WIDGET(remote_row));
    }
    i++;
    remote_row = gtk_list_box_get_row_at_index (GTK_LIST_BOX(box), i);
  }
  // the last row got removed and we have to display the searching box
  if (i <= 2) {
    gtk_widget_show (priv->remote_no_devices);
  }
}

GtkWidget *
find_child(GtkWidget *parent, const gchar *name)
{
  if (g_strcmp0(gtk_widget_get_name((GtkWidget *)parent), (gchar *)name) == 0) {
    return parent;
  }

  if (GTK_IS_BIN(parent)) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
    return find_child(child, name);
  }

  if (GTK_IS_CONTAINER(parent)) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
    while ((children = g_list_next(children)) != NULL) {
      GtkWidget *widget = find_child(children->data, name);
      if (widget != NULL) {
        return widget;
      }
    }
  }

  return NULL;
}


static void
teleport_window_dispose (GObject *object)
{
  TeleportWindow *win;
  TeleportWindowPrivate *priv;

  win = TELEPORT_WINDOW (object);
  priv = teleport_window_get_instance_private (win);

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (teleport_window_parent_class)->dispose (object);
}

static void
teleport_window_class_init (TeleportWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = teleport_window_dispose;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/com/frac_tion/teleport/window.ui");

  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, gears);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, this_device_name_label);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_no_devices);
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class), TeleportWindow, remote_devices_list);
}

TeleportWindow *
teleport_window_new (TeleportApp *app)
{
  return g_object_new (TELEPORT_WINDOW_TYPE, "application", app, NULL);
}

gchar * 
teleport_get_device_name (void) 
{
  TeleportWindowPrivate *priv;
  priv = teleport_window_get_instance_private (mainWin);

  return g_settings_get_string (priv->settings, "device-name");
}

gchar * 
teleport_get_download_directory (void) 
{
  TeleportWindowPrivate *priv;
  priv = teleport_window_get_instance_private (mainWin);

  return g_settings_get_string (priv->settings, "download-dir");
}

void
teleport_window_open (TeleportWindow *win,
                      GFile *file)
{
  //TeleportWindowPrivate *priv;
  //priv = teleport_window_get_instance_private (win);
}
