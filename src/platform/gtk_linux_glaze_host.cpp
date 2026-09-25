#include "goreecloud/browser/platform/gtk_linux_glaze_host.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "goreecloud/browser/glaze.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/live_media_hover_coordinator.hpp"
#include "goreecloud/browser/media_hit_test_provider.hpp"
#include "goreecloud/browser/native_engine_surface.hpp"
#include "goreecloud/browser/platform/gtk_media_hover_popover.hpp"

namespace goreecloud::browser::platform {
namespace {

inline constexpr int kGlazeInteractiveTargetPx =
    static_cast<int>(kBrowserGlazeCapabilities.minimum_target_px);

void set_accessible_name(GtkWidget* widget, const char* name) {
  gtk_widget_set_tooltip_text(widget, name);
  if (auto* accessible = gtk_widget_get_accessible(widget)) atk_object_set_name(accessible, name);
}

GtkWidget* make_icon_or_label(const char* icon_name, const char* fallback_label) {
  auto* theme = gtk_icon_theme_get_default();
  if (theme && icon_name && gtk_icon_theme_has_icon(theme, icon_name)) {
    return gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
  }

  auto* label = gtk_label_new(fallback_label);
  gtk_style_context_add_class(gtk_widget_get_style_context(label), "gc-button-glyph");
  return label;
}

GtkWidget* make_toolbar_button(const char* icon_name,
                               const char* fallback_label,
                               const char* accessible_name) {
  auto* button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(button), make_icon_or_label(icon_name, fallback_label));
  gtk_widget_set_size_request(button, kGlazeInteractiveTargetPx, kGlazeInteractiveTargetPx);
  gtk_style_context_add_class(gtk_widget_get_style_context(button), "gc-toolbar-button");
  set_accessible_name(button, accessible_name);
  return button;
}

struct InternalSurfaceCopy {
  std::string eyebrow;
  std::string title;
  std::string body;
};

InternalSurfaceCopy internal_surface_copy(std::string_view url) {
  if (url == kNewTabUrl) {
    return {"GOREECLOUD BROWSER", "New Tab",
            "Search with GoreeCloud Search or enter an address. Direct HTTP(S) navigation remains independent from search availability."};
  }
  if (url == kHomeUrl) {
    return {"GOREECLOUD BROWSER", "Home",
            "Your first-party GoreeCloud Browser home surface."};
  }
  if (url == kSettingsUrl) {
    return {"BROWSER SETTINGS", "Settings",
            "Browser-owned settings use the current Glaze UI presentation contract while underlying privacy, security, identity, and policy truth remains provider-owned."};
  }
  if (url == kPrivateStartUrl) {
    return {"PRIVATE BROWSING", "Browse without local history persistence",
            "This window uses temporary local browsing state. Privacy Shield and Wardveil Security remain authoritative for their own protection state. Private Browsing is not a guarantee of network anonymity."};
  }
  return {"GOREECLOUD BROWSER", "Browser Surface", std::string{url}};
}

}  // namespace

class GtkLinuxGlazeWindowHost::Impl {
 public:
  Impl() {
    media_hover.set_present_callback(
        [this](const MediaTarget& target, const MediaHoverViewModel& model, MediaHitTestPoint point) {
          current_media_target = target;
          if (!content_area) return;
          show_gtk_media_hover_popover(
              content_area,
              model,
              point.viewport_x,
              point.viewport_y,
              [this](MediaAction action) {
                if (media_hover_action_handler && current_media_target) {
                  media_hover_action_handler(action, *current_media_target);
                }
              });
        });
    media_hover.set_hide_callback([this]() {
      current_media_target.reset();
      if (content_area) hide_gtk_media_hover_popover(content_area);
    });
  }

  static void on_window_destroy(GtkWidget*, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    self->stop_media_hover_timer();
    self->media_hover.invalidate();
    if (self->content_area) hide_gtk_media_hover_popover(self->content_area);
    self->close_requested = true;
    self->window = nullptr;
  }

  static gboolean on_media_hover_tick(gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!self->window || self->close_requested) return G_SOURCE_REMOVE;
    self->sample_media_hover();
    return G_SOURCE_CONTINUE;
  }

  static void on_toolbar_clicked(GtkButton* button, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (self->overflow_popover) gtk_widget_hide(self->overflow_popover);
    const auto found = self->toolbar_bindings.find(GTK_WIDGET(button));
    if (found != self->toolbar_bindings.end() && self->toolbar_handler) self->toolbar_handler(found->second);
  }

  static void on_search_control_clicked(GtkButton* button, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    const auto found = self->search_control_bindings.find(GTK_WIDGET(button));
    if (found != self->search_control_bindings.end() && self->search_control_handler) self->search_control_handler(found->second);
  }

  static void on_search_activate(GtkEntry* entry, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!self->search_handler) return;
    const char* value = gtk_entry_get_text(entry);
    self->search_handler(value ? std::string_view{value} : std::string_view{});
  }

  static void on_content_size_allocate(GtkWidget*, GtkAllocation*, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    self->update_metrics();
    self->resize_attached_engine();
    self->media_hover.invalidate();
    if (self->content_area) hide_gtk_media_hover_popover(self->content_area);
  }

  void sample_media_hover() {
    if (!engine_surface_attached || !attached_view || !content_area || !gtk_widget_get_realized(content_area)) {
      media_hover.pointer_left_content();
      return;
    }

    auto* provider = dynamic_cast<AsyncMediaHitTestProvider*>(attached_view);
    if (!provider) {
      media_hover.pointer_left_content();
      return;
    }

    auto* gdk_window = gtk_widget_get_window(content_area);
    if (!gdk_window) return;
    auto* display = gdk_window_get_display(gdk_window);
    auto* seat = display ? gdk_display_get_default_seat(display) : nullptr;
    auto* pointer = seat ? gdk_seat_get_pointer(seat) : nullptr;
    if (!pointer) return;

    gint x = 0;
    gint y = 0;
    GdkModifierType mask{};
    gdk_window_get_device_position(gdk_window, pointer, &x, &y, &mask);

    GtkAllocation allocation{};
    gtk_widget_get_allocation(content_area, &allocation);
    if (x < 0 || y < 0 || x >= allocation.width || y >= allocation.height) {
      media_hover.pointer_left_content();
      return;
    }

    media_hover.probe(*provider, MediaHitTestPoint{.viewport_x = x, .viewport_y = y});
  }

  void start_media_hover_timer() {
    if (!media_hover_timer_id) media_hover_timer_id = g_timeout_add(75, on_media_hover_tick, this);
  }

  void stop_media_hover_timer() {
    if (media_hover_timer_id) {
      g_source_remove(media_hover_timer_id);
      media_hover_timer_id = 0;
    }
  }

  void install_css() {
    css = gtk_css_provider_new();
    static constexpr const char* kCss = R"CSS(
      window.gc-browser-window {
        background-color: @theme_base_color;
        color: @theme_fg_color;
      }

      .gc-tab-strip {
        min-height: 46px;
        padding: 8px 12px 4px;
        background-color: alpha(@theme_bg_color, 0.98);
      }

      .gc-tab-capsule {
        min-height: 34px;
        min-width: 180px;
        padding: 5px 14px;
        border-radius: 14px 14px 8px 8px;
        border: 1px solid alpha(@theme_fg_color, 0.12);
        background-color: @theme_base_color;
      }

      .gc-tab-capsule.gc-loading {
        border-bottom-color: @theme_selected_bg_color;
        border-bottom-width: 2px;
      }

      .gc-tab-title {
        font-weight: 600;
      }

      .gc-private-badge {
        padding: 5px 10px;
        border-radius: 14px;
        border: 1px solid alpha(@theme_selected_bg_color, 0.36);
        background-color: alpha(@theme_selected_bg_color, 0.10);
      }

      .gc-toolbar {
        padding: 4px 12px 10px;
        background-color: alpha(@theme_bg_color, 0.98);
      }

      .gc-control-cluster {
        padding: 2px;
        border-radius: 18px;
        border: 1px solid alpha(@theme_fg_color, 0.10);
        background-color: alpha(@theme_base_color, 0.82);
      }

      button.gc-toolbar-button,
      button.gc-search-control,
      button.gc-overflow-button {
        min-width: 48px;
        min-height: 48px;
        padding: 0;
        border-radius: 16px;
        border: 1px solid transparent;
        background: transparent;
        box-shadow: none;
      }

      button.gc-toolbar-button:hover,
      button.gc-search-control:hover,
      button.gc-overflow-button:hover {
        background-color: alpha(@theme_fg_color, 0.07);
      }

      button.gc-toolbar-button:active,
      button.gc-search-control:active,
      button.gc-overflow-button:active {
        background-color: alpha(@theme_fg_color, 0.12);
      }

      button.gc-toolbar-button:focus,
      button.gc-search-control:focus,
      button.gc-overflow-button:focus,
      button.gc-overflow-action:focus {
        border-color: @theme_selected_bg_color;
        box-shadow: 0 0 0 1px @theme_selected_bg_color;
      }

      .gc-button-glyph {
        font-size: 18px;
        font-weight: 600;
      }

      .gc-search-shell {
        min-height: 52px;
        margin-left: 4px;
        margin-right: 4px;
        padding: 2px 5px 2px 12px;
        border-radius: 26px;
        border: 1px solid alpha(@theme_fg_color, 0.14);
        background-color: alpha(@theme_base_color, 0.96);
      }

      entry.gc-search-entry {
        min-height: 46px;
        padding-left: 6px;
        padding-right: 8px;
        border: none;
        border-radius: 20px;
        box-shadow: none;
        background: transparent;
      }

      entry.gc-search-entry:focus {
        box-shadow: inset 0 -2px @theme_selected_bg_color;
      }

      .gc-overflow-popover {
        padding: 6px;
      }

      button.gc-overflow-action {
        min-height: 44px;
        padding: 8px 14px;
        border-radius: 12px;
        border: 1px solid transparent;
        background: transparent;
      }

      button.gc-overflow-action:hover {
        background-color: alpha(@theme_fg_color, 0.07);
      }

      .gc-internal-surface,
      .gc-panel-surface {
        min-width: 520px;
        padding: 32px 36px;
        border-radius: 24px;
        border: 1px solid alpha(@theme_fg_color, 0.12);
        background-color: alpha(@theme_base_color, 0.98);
      }

      .gc-surface-eyebrow {
        font-size: 11px;
        font-weight: 700;
        letter-spacing: 0.10em;
        color: alpha(@theme_fg_color, 0.62);
      }

      .gc-surface-title {
        font-size: 26px;
        font-weight: 700;
      }

      .gc-surface-body {
        font-size: 15px;
        color: alpha(@theme_fg_color, 0.78);
      }

      window.gc-private-window .gc-toolbar,
      window.gc-private-window .gc-tab-strip {
        background-color: alpha(@theme_selected_bg_color, 0.055);
      }
    )CSS";
    gtk_css_provider_load_from_data(css, kCss, -1, nullptr);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }

  bool create() {
    if (created) return true;
    if (!gtk_init_check(nullptr, nullptr)) return false;
    auto* display = gdk_display_get_default();
    if (!display || !GDK_IS_X11_DISPLAY(display)) return false;

    install_css();
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);
    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Browser");
    gtk_style_context_add_class(gtk_widget_get_style_context(window), "gc-browser-window");
    if (private_window) gtk_style_context_add_class(gtk_widget_get_style_context(window), "gc-private-window");
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root);

    tab_strip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(tab_strip), "gc-tab-strip");
    tab_capsule = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(tab_capsule), "gc-tab-capsule");
    tab_label = gtk_label_new("New Tab");
    gtk_label_set_ellipsize(GTK_LABEL(tab_label), PANGO_ELLIPSIZE_END);
    gtk_label_set_max_width_chars(GTK_LABEL(tab_label), 48);
    gtk_widget_set_halign(tab_label, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(tab_label), "gc-tab-title");
    gtk_box_pack_start(GTK_BOX(tab_capsule), tab_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_strip), tab_capsule, FALSE, FALSE, 0);

    if (private_window) {
      private_badge = gtk_label_new("Private");
      gtk_style_context_add_class(gtk_widget_get_style_context(private_badge), "gc-private-badge");
      set_accessible_name(private_badge, "Private Browsing window");
      gtk_box_pack_end(GTK_BOX(tab_strip), private_badge, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(root), tab_strip, FALSE, FALSE, 0);

    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(toolbar), "gc-toolbar");
    gtk_box_pack_start(GTK_BOX(root), toolbar, FALSE, FALSE, 0);

    navigation_cluster = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_style_context_add_class(gtk_widget_get_style_context(navigation_cluster), "gc-control-cluster");
    gtk_box_pack_start(GTK_BOX(toolbar), navigation_cluster, FALSE, FALSE, 0);
    add_toolbar_button(ToolbarItem::back, "go-previous-symbolic", "←", "Back", navigation_cluster);
    add_toolbar_button(ToolbarItem::forward, "go-next-symbolic", "→", "Forward", navigation_cluster);
    add_toolbar_button(ToolbarItem::refresh, "view-refresh-symbolic", "↻", "Refresh or Stop", navigation_cluster);
    add_toolbar_button(ToolbarItem::home, "go-home-symbolic", "⌂", "Home", navigation_cluster);

    build_unified_search();

    status_cluster = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_style_context_add_class(gtk_widget_get_style_context(status_cluster), "gc-control-cluster");
    gtk_box_pack_start(GTK_BOX(toolbar), status_cluster, FALSE, FALSE, 0);
    add_toolbar_button(ToolbarItem::privacy_shield, "security-medium-symbolic", "P", "Privacy Shield", status_cluster);
    add_toolbar_button(ToolbarItem::wardveil_security, "security-high-symbolic", "W", "Wardveil Security", status_cluster);
    build_overflow_menu();

    content_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(content_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_box_pack_start(GTK_BOX(root), content_stack, TRUE, TRUE, 0);

    content_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(content_area, TRUE);
    gtk_widget_set_vexpand(content_area, TRUE);
    gtk_stack_add_named(GTK_STACK(content_stack), content_area, "web");
    g_signal_connect(content_area, "size-allocate", G_CALLBACK(on_content_size_allocate), this);

    internal_surface = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(internal_surface, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(internal_surface, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(internal_surface, 24);
    gtk_widget_set_margin_end(internal_surface, 24);
    gtk_style_context_add_class(gtk_widget_get_style_context(internal_surface), "gc-internal-surface");

    internal_eyebrow = gtk_label_new(nullptr);
    gtk_widget_set_halign(internal_eyebrow, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(internal_eyebrow), "gc-surface-eyebrow");
    gtk_box_pack_start(GTK_BOX(internal_surface), internal_eyebrow, FALSE, FALSE, 0);

    internal_title = gtk_label_new(nullptr);
    gtk_widget_set_halign(internal_title, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(internal_title), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(internal_title), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(internal_title), "gc-surface-title");
    gtk_box_pack_start(GTK_BOX(internal_surface), internal_title, FALSE, FALSE, 0);

    internal_body = gtk_label_new(nullptr);
    gtk_widget_set_halign(internal_body, GTK_ALIGN_FILL);
    gtk_label_set_xalign(GTK_LABEL(internal_body), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(internal_body), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(internal_body), 72);
    gtk_style_context_add_class(gtk_widget_get_style_context(internal_body), "gc-surface-body");
    gtk_box_pack_start(GTK_BOX(internal_surface), internal_body, FALSE, FALSE, 0);
    gtk_stack_add_named(GTK_STACK(content_stack), internal_surface, "internal");

    panel_surface = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(panel_surface, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(panel_surface, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_start(panel_surface, 24);
    gtk_widget_set_margin_end(panel_surface, 24);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel_surface), "gc-panel-surface");

    panel_eyebrow = gtk_label_new("BROWSER TOOL");
    gtk_widget_set_halign(panel_eyebrow, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel_eyebrow), "gc-surface-eyebrow");
    gtk_box_pack_start(GTK_BOX(panel_surface), panel_eyebrow, FALSE, FALSE, 0);

    panel_title = gtk_label_new(nullptr);
    gtk_widget_set_halign(panel_title, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(panel_title), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(panel_title), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel_title), "gc-surface-title");
    gtk_box_pack_start(GTK_BOX(panel_surface), panel_title, FALSE, FALSE, 0);

    panel_body = gtk_label_new(nullptr);
    gtk_widget_set_halign(panel_body, GTK_ALIGN_FILL);
    gtk_label_set_xalign(GTK_LABEL(panel_body), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(panel_body), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(panel_body), 78);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel_body), "gc-surface-body");
    gtk_box_pack_start(GTK_BOX(panel_surface), panel_body, FALSE, FALSE, 0);
    gtk_stack_add_named(GTK_STACK(content_stack), panel_surface, "panel");

    start_media_hover_timer();
    created = true;
    return true;
  }

  void add_toolbar_button(ToolbarItem item,
                          const char* icon_name,
                          const char* fallback_label,
                          const char* accessible,
                          GtkWidget* container) {
    auto* button = make_toolbar_button(icon_name, fallback_label, accessible);
    toolbar_bindings.emplace(button, item);
    g_signal_connect(button, "clicked", G_CALLBACK(on_toolbar_clicked), this);
    gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);
  }

  void build_unified_search() {
    search_shell = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(search_shell), "gc-search-shell");
    gtk_widget_set_hexpand(search_shell, TRUE);
    set_accessible_name(search_shell, "Unified Search Bar");

    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search with GoreeCloud Search or enter address");
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(search_entry), "gc-search-entry");
    set_accessible_name(search_entry, "Unified Search Bar");
    g_signal_connect(search_entry, "activate", G_CALLBACK(on_search_activate), this);
    gtk_box_pack_start(GTK_BOX(search_shell), search_entry, TRUE, TRUE, 0);

    add_search_control(UnifiedSearchBarControl::advanced_reader_mode,
                       "accessories-text-editor-symbolic", "R", "Advanced Reader Mode");
    add_search_control(UnifiedSearchBarControl::unified_bookmarks,
                       "starred-symbolic", "★", "Unified Bookmarks");
    add_search_control(UnifiedSearchBarControl::wardveil_security,
                       "security-high-symbolic", "W", "Wardveil Security");
    gtk_box_pack_start(GTK_BOX(toolbar), search_shell, TRUE, TRUE, 0);
  }

  void add_search_control(UnifiedSearchBarControl control,
                          const char* icon_name,
                          const char* fallback_label,
                          const char* accessible) {
    auto* button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), make_icon_or_label(icon_name, fallback_label));
    gtk_widget_set_size_request(button, kGlazeInteractiveTargetPx, kGlazeInteractiveTargetPx);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "gc-search-control");
    set_accessible_name(button, accessible);
    search_control_bindings.emplace(button, control);
    g_signal_connect(button, "clicked", G_CALLBACK(on_search_control_clicked), this);
    gtk_box_pack_start(GTK_BOX(search_shell), button, FALSE, FALSE, 0);
  }

  void build_overflow_menu() {
    more_button = gtk_menu_button_new();
    gtk_container_add(GTK_CONTAINER(more_button), make_icon_or_label("open-menu-symbolic", "⋮"));
    gtk_widget_set_size_request(more_button, kGlazeInteractiveTargetPx, kGlazeInteractiveTargetPx);
    gtk_style_context_add_class(gtk_widget_get_style_context(more_button), "gc-overflow-button");
    set_accessible_name(more_button, "More Browser tools");

    overflow_popover = gtk_popover_new(more_button);
    gtk_style_context_add_class(gtk_widget_get_style_context(overflow_popover), "gc-overflow-popover");
    overflow_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_add(GTK_CONTAINER(overflow_popover), overflow_box);
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(more_button), overflow_popover);

    add_overflow_action(ToolbarItem::advanced_download_manager, "Downloads");
    add_overflow_action(ToolbarItem::clipboard, "Clipboard");
    add_overflow_action(ToolbarItem::dns_cache, "Clear DNS Cache");
    add_overflow_action(ToolbarItem::advanced_proxy_manager, "Advanced Proxy Manager");
    add_overflow_action(ToolbarItem::settings, "Settings");

    gtk_box_pack_end(GTK_BOX(toolbar), more_button, FALSE, FALSE, 0);
  }

  void add_overflow_action(ToolbarItem item, const char* label) {
    auto* button = gtk_button_new_with_label(label);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "gc-overflow-action");
    set_accessible_name(button, label);
    toolbar_bindings.emplace(button, item);
    g_signal_connect(button, "clicked", G_CALLBACK(on_toolbar_clicked), this);
    gtk_box_pack_start(GTK_BOX(overflow_box), button, FALSE, FALSE, 0);
  }

  void show() {
    if (!created || !window) return;
    gtk_widget_show_all(window);
    gtk_widget_realize(content_area);
    update_metrics();
    if (attached_view) attach_engine_surface();
  }

  NativeEngineSurface current_surface() const {
    NativeEngineSurface surface;
    if (!content_area || !gtk_widget_get_realized(content_area)) return surface;
    auto* gdk_window = gtk_widget_get_window(content_area);
    if (!gdk_window) return surface;
    auto* display = gdk_window_get_display(gdk_window);
    if (!display || !GDK_IS_X11_DISPLAY(display)) return surface;
    GtkAllocation allocation{};
    gtk_widget_get_allocation(content_area, &allocation);
    surface.window_handle = static_cast<std::uintptr_t>(gdk_x11_window_get_xid(gdk_window));
    surface.display_handle = reinterpret_cast<std::uintptr_t>(gdk_x11_display_get_xdisplay(display));
    surface.x = 0;
    surface.y = 0;
    surface.width = allocation.width;
    surface.height = allocation.height;
    surface.scale_factor = static_cast<float>(gtk_widget_get_scale_factor(content_area));
    return surface;
  }

  void attach_engine_surface() {
    if (!attached_view || !content_area || !gtk_widget_get_realized(content_area)) return;
    auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(attached_view);
    if (!attachable) return;
    const auto surface = current_surface();
    if (surface.window_handle == 0 || surface.width <= 0 || surface.height <= 0) return;
    engine_surface_attached = attachable->attach_native_surface(surface);
    if (engine_surface_attached) gtk_stack_set_visible_child_name(GTK_STACK(content_stack), "web");
  }

  void resize_attached_engine() {
    if (!engine_surface_attached || !attached_view) return;
    auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(attached_view);
    if (!attachable) return;
    const auto surface = current_surface();
    if (surface.window_handle != 0 && surface.width > 0 && surface.height > 0) attachable->resize_native_surface(surface);
  }

  void update_metrics() {
    if (!window) return;
    int width = 0;
    int height = 0;
    gtk_window_get_size(GTK_WINDOW(window), &width, &height);
    metrics.width = width;
    metrics.height = height;
    metrics.scale_factor = static_cast<float>(gtk_widget_get_scale_factor(window));
  }

  ToolbarHandler toolbar_handler;
  SearchHandler search_handler;
  SearchControlHandler search_control_handler;
  MediaHoverActionHandler media_hover_action_handler;
  LiveMediaHoverCoordinator media_hover;
  std::optional<MediaTarget> current_media_target;
  std::unordered_map<GtkWidget*, ToolbarItem> toolbar_bindings;
  std::unordered_map<GtkWidget*, UnifiedSearchBarControl> search_control_bindings;
  GtkWidget* window{nullptr};
  GtkWidget* root{nullptr};
  GtkWidget* tab_strip{nullptr};
  GtkWidget* tab_capsule{nullptr};
  GtkWidget* tab_label{nullptr};
  GtkWidget* private_badge{nullptr};
  GtkWidget* toolbar{nullptr};
  GtkWidget* navigation_cluster{nullptr};
  GtkWidget* status_cluster{nullptr};
  GtkWidget* search_shell{nullptr};
  GtkWidget* search_entry{nullptr};
  GtkWidget* more_button{nullptr};
  GtkWidget* overflow_popover{nullptr};
  GtkWidget* overflow_box{nullptr};
  GtkWidget* content_stack{nullptr};
  GtkWidget* content_area{nullptr};
  GtkWidget* internal_surface{nullptr};
  GtkWidget* internal_eyebrow{nullptr};
  GtkWidget* internal_title{nullptr};
  GtkWidget* internal_body{nullptr};
  GtkWidget* panel_surface{nullptr};
  GtkWidget* panel_eyebrow{nullptr};
  GtkWidget* panel_title{nullptr};
  GtkWidget* panel_body{nullptr};
  GtkCssProvider* css{nullptr};
  EngineView* attached_view{nullptr};
  NativeWindowMetrics metrics{1280, 800, 1.0F};
  guint media_hover_timer_id{0};
  bool private_window{false};
  bool created{false};
  bool close_requested{false};
  bool engine_surface_attached{false};
};

GtkLinuxGlazeWindowHost::GtkLinuxGlazeWindowHost() : impl_(std::make_unique<Impl>()) {}

GtkLinuxGlazeWindowHost::~GtkLinuxGlazeWindowHost() {
  if (!impl_) return;
  impl_->stop_media_hover_timer();
  detach_engine_view();
  if (impl_->css) {
    g_object_unref(impl_->css);
    impl_->css = nullptr;
  }
}

void GtkLinuxGlazeWindowHost::set_toolbar_handler(ToolbarHandler handler) { impl_->toolbar_handler = std::move(handler); }
void GtkLinuxGlazeWindowHost::set_search_handler(SearchHandler handler) { impl_->search_handler = std::move(handler); }
void GtkLinuxGlazeWindowHost::set_search_control_handler(SearchControlHandler handler) { impl_->search_control_handler = std::move(handler); }
void GtkLinuxGlazeWindowHost::set_media_hover_action_handler(MediaHoverActionHandler handler) { impl_->media_hover_action_handler = std::move(handler); }
void GtkLinuxGlazeWindowHost::set_media_hover_policy(MediaHoverSitePolicy policy) { impl_->media_hover.set_policy(std::move(policy)); }
void GtkLinuxGlazeWindowHost::set_private_window(bool private_window) { impl_->private_window = private_window; }

bool GtkLinuxGlazeWindowHost::create() { return impl_->create(); }
void GtkLinuxGlazeWindowHost::show() { impl_->show(); }

void GtkLinuxGlazeWindowHost::close() {
  impl_->stop_media_hover_timer();
  detach_engine_view();
  if (impl_->window) {
    gtk_widget_destroy(impl_->window);
    impl_->window = nullptr;
  }
  impl_->close_requested = true;
}

void GtkLinuxGlazeWindowHost::set_title(std::string_view title) {
  if (impl_->window) gtk_window_set_title(GTK_WINDOW(impl_->window), std::string{title}.c_str());
}

void GtkLinuxGlazeWindowHost::render_chrome(const BrowserChromeState& state) {
  if (impl_->search_entry && !gtk_widget_has_focus(impl_->search_entry)) {
    gtk_entry_set_text(GTK_ENTRY(impl_->search_entry), state.unified_search.display_text.c_str());
  }

  if (impl_->tab_label && !state.tabs.empty()) {
    const auto& tab = state.tabs.front();
    gtk_label_set_text(GTK_LABEL(impl_->tab_label), tab.title.empty() ? "New Tab" : tab.title.c_str());
    if (impl_->tab_capsule) {
      auto* context = gtk_widget_get_style_context(impl_->tab_capsule);
      if (tab.loading) {
        gtk_style_context_add_class(context, "gc-loading");
      } else {
        gtk_style_context_remove_class(context, "gc-loading");
      }
    }
  }
}

void GtkLinuxGlazeWindowHost::attach_engine_view(EngineView& view) {
  detach_engine_view();
  impl_->attached_view = &view;
  impl_->media_hover.invalidate();
  impl_->attach_engine_surface();
}

void GtkLinuxGlazeWindowHost::detach_engine_view() {
  impl_->media_hover.invalidate();
  impl_->current_media_target.reset();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (impl_->attached_view) {
    if (auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(impl_->attached_view)) {
      if (attachable->native_surface_attached()) attachable->detach_native_surface();
    }
  }
  impl_->attached_view = nullptr;
  impl_->engine_surface_attached = false;
}

void GtkLinuxGlazeWindowHost::show_internal_surface(std::string_view internal_url) {
  impl_->media_hover.invalidate();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (!impl_->content_stack || !impl_->internal_eyebrow || !impl_->internal_title ||
      !impl_->internal_body) {
    return;
  }

  const auto copy = internal_surface_copy(internal_url);
  gtk_label_set_text(GTK_LABEL(impl_->internal_eyebrow), copy.eyebrow.c_str());
  gtk_label_set_text(GTK_LABEL(impl_->internal_title), copy.title.c_str());
  gtk_label_set_text(GTK_LABEL(impl_->internal_body), copy.body.c_str());
  gtk_stack_set_visible_child_name(GTK_STACK(impl_->content_stack), "internal");
}

void GtkLinuxGlazeWindowHost::show_panel(std::string_view panel_id) {
  impl_->media_hover.invalidate();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (!impl_->content_stack || !impl_->panel_title || !impl_->panel_body) return;

  const auto line_break = panel_id.find('\n');
  const std::string title{
      line_break == std::string_view::npos ? panel_id : panel_id.substr(0, line_break)};
  const std::string body =
      line_break == std::string_view::npos
          ? "Browser-owned tool surface. Availability and status remain bound to the underlying authoritative capability."
          : std::string{panel_id.substr(line_break + 1)};

  gtk_label_set_text(GTK_LABEL(impl_->panel_title), title.c_str());
  gtk_label_set_text(GTK_LABEL(impl_->panel_body), body.c_str());
  gtk_stack_set_visible_child_name(GTK_STACK(impl_->content_stack), "panel");
}

NativeWindowMetrics GtkLinuxGlazeWindowHost::metrics() const { return impl_->metrics; }

bool GtkLinuxGlazeWindowHost::pump_events() {
  while (gtk_events_pending()) gtk_main_iteration_do(FALSE);
  return !impl_->close_requested;
}

bool GtkLinuxGlazeWindowHost::close_requested() const noexcept { return impl_->close_requested; }

}  // namespace goreecloud::browser::platform