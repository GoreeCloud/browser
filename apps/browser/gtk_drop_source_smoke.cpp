#include <gtk/gtk.h>

namespace {

void on_drag_data_get(GtkWidget*,
                      GdkDragContext*,
                      GtkSelectionData* selection_data,
                      guint,
                      guint,
                      gpointer) {
  constexpr const char* kPayload = "goreecloud-drop";
  gtk_selection_data_set_text(selection_data, kPayload, -1);
}

}  // namespace

int main(int argc, char** argv) {
  if (!gtk_init_check(&argc, &argv)) return 2;

  auto* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Drag Source");
  gtk_window_set_default_size(GTK_WINDOW(window), 240, 160);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

  auto* label = gtk_label_new("Drag GoreeCloud text");
  gtk_widget_set_size_request(label, 220, 140);
  gtk_container_add(GTK_CONTAINER(window), label);

  GtkTargetEntry targets[] = {
      {const_cast<gchar*>("text/plain;charset=utf-8"), 0, 0},
      {const_cast<gchar*>("UTF8_STRING"), 0, 0},
      {const_cast<gchar*>("text/plain"), 0, 0},
  };
  gtk_drag_source_set(label, GDK_BUTTON1_MASK, targets,
                      static_cast<gint>(G_N_ELEMENTS(targets)),
                      GDK_ACTION_COPY);
  g_signal_connect(label, "drag-data-get",
                   G_CALLBACK(on_drag_data_get), nullptr);

  gtk_widget_show_all(window);
  gtk_main();
  return 0;
}
