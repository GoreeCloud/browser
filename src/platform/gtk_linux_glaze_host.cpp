#include "goreecloud/browser/platform/gtk_linux_glaze_host.hpp"

#include <cstdint>
#include <string>
#include <string_view>
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

void add_style_class(GtkWidget* widget, const char* class_name) {
  gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

void set_accessible_name(GtkWidget* widget, const char* name) {
  gtk_widget_set_tooltip_text(widget, name);
  if (auto* accessible = gtk_widget_get_accessible(widget)) {
    atk_object_set_name(accessible, name);
  }
}

GtkWidget* make_toolbar_button(const char* visible_label,
                               const char* accessible_name) {
  auto* button = gtk_button_new_with_label(visible_label);
  gtk_widget_set_size_request(button, kGlazeInteractiveTargetPx,
                              kGlazeInteractiveTargetPx);
  add_style_class(button, "gc-toolbar-button");
  set_accessible_name(button, accessible_name);
  return button;
}

GtkWidget* make_brand_mark() {
  constexpr const char* kBrandAsset = "assets/branding/goreecloud-browser.svg";
  GError* error = nullptr;
  if (auto* pixbuf =
          gdk_pixbuf_new_from_file_at_scale(kBrandAsset, 28, 28, TRUE, &error)) {
    auto* image = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    add_style_class(image, "gc-brand-mark");
    set_accessible_name(image, "GoreeCloud Browser");
    return image;
  }
  if (error) g_error_free(error);

  auto* fallback = gtk_label_new("G");
  add_style_class(fallback, "gc-brand-fallback");
  set_accessible_name(fallback, "GoreeCloud Browser");
  return fallback;
}

GtkWidget* make_status_chip(const char* text) {
  auto* label = gtk_label_new(text);
  add_style_class(label, "gc-status-chip");
  return label;
}

struct InternalSurfaceCopy {
  const char* eyebrow;
  const char* title;
  const char* subtitle;
  const char* status;
};

InternalSurfaceCopy internal_surface_copy(std::string_view url) {
  if (url == kNewTabUrl) {
    return {"GOREECLOUD BROWSER", "A focused place to start.",
            "Search with GoreeCloud Search or enter an address in the navigation capsule above.",
            "Desktop renderer integration pending"};
  }
  if (url == kHomeUrl) {
    return {"GOREECLOUD HOME", "Your browser, your workspace.",
            "Home is a first-party GoreeCloud surface. Rich modules and synchronized content remain development work.",
            "Development surface"};
  }
  if (url == kSettingsUrl) {
    return {"BROWSER SETTINGS", "Control the browser without losing context.",
            "Settings will grow into grouped privacy, security, appearance, search, downloads, network, permissions, and data controls.",
            "Settings surface under development"};
  }
  if (url == kPrivateStartUrl) {
    return {"PRIVATE BROWSING", "Temporary browsing, clearly separated.",
            "Private state is kept separate from ordinary persistence. Privacy Shield and Wardveil Security remain authoritative for their actual protection state.",
            "Private runtime acceptance pending"};
  }
  return {"GOREECLOUD BROWSER", "First-party browser surface",
          "This GoreeCloud-owned internal destination is still being implemented.",
          "Development surface"};
}

}  // namespace

class GtkLinuxGlazeWindowHost::Impl {
 public:
  Impl() {
    media_hover.set_present_callback(
        [this](const MediaTarget& target, const MediaHoverViewModel& model,
               MediaHitTestPoint point) {
          current_media_target = target;
          if (!content_area) return;
          show_gtk_media_hover_popover(
              content_area, model, point.viewport_x, point.viewport_y,
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
    const auto found = self->toolbar_bindings.find(GTK_WIDGET(button));
    if (found != self->toolbar_bindings.end() && self->toolbar_handler) {
      self->toolbar_handler(found->second);
    }
  }

  static void on_search_control_clicked(GtkButton* button, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    const auto found =
        self->search_control_bindings.find(GTK_WIDGET(button));
    if (found != self->search_control_bindings.end() &&
        self->search_control_handler) {
      self->search_control_handler(found->second);
    }
  }

  static void on_search_activate(GtkEntry* entry, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!self->search_handler) return;
    const char* value = gtk_entry_get_text(entry);
    self->search_handler(value ? std::string_view{value} : std::string_view{});
  }

  static void on_content_size_allocate(GtkWidget*, GtkAllocation*,
                                       gpointer data) {
    auto* self = static_cast<Impl*>(data);
    self->update_metrics();
    self->resize_attached_engine();
    self->media_hover.invalidate();
    if (self->content_area) hide_gtk_media_hover_popover(self->content_area);
  }

  void sample_media_hover() {
    if (!engine_surface_attached || !attached_view || !content_area ||
        !gtk_widget_get_realized(content_area)) {
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
    media_hover.probe(*provider,
                      MediaHitTestPoint{.viewport_x = x, .viewport_y = y});
  }

  void start_media_hover_timer() {
    if (!media_hover_timer_id) {
      media_hover_timer_id = g_timeout_add(75, on_media_hover_tick, this);
    }
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
      .gc-chrome-shell {
        background-color: alpha(@theme_bg_color, 0.97);
        border-bottom: 1px solid alpha(@theme_fg_color, 0.10);
        box-shadow: 0 5px 18px alpha(#000000, 0.08);
      }
      .gc-tab-strip {
        min-height: 42px;
        padding: 8px 14px 4px 14px;
      }
      .gc-brand-mark, .gc-brand-fallback {
        min-width: 30px;
        min-height: 30px;
        margin-right: 4px;
        font-weight: 800;
      }
      .gc-active-tab {
        min-height: 36px;
        padding: 0 14px;
        border-radius: 13px;
        background-color: alpha(@theme_base_color, 0.88);
        border: 1px solid alpha(@theme_fg_color, 0.10);
        box-shadow: 0 2px 8px alpha(#000000, 0.06);
      }
      .gc-tab-title { font-weight: 600; }
      .gc-tab-status { opacity: 0.66; font-size: 0.88em; }
      .gc-stage-badge {
        padding: 5px 10px;
        border-radius: 999px;
        background-color: alpha(@theme_selected_bg_color, 0.10);
        border: 1px solid alpha(@theme_selected_bg_color, 0.22);
        color: @theme_fg_color;
        font-size: 0.86em;
      }
      .gc-toolbar {
        min-height: 60px;
        padding: 4px 14px 11px 14px;
      }
      .gc-toolbar-button, .gc-overflow-button {
        min-width: 48px;
        min-height: 48px;
        padding: 0;
        border-radius: 16px;
        border: 1px solid transparent;
        background: transparent;
        box-shadow: none;
      }
      .gc-toolbar-button:hover, .gc-overflow-button:hover {
        background-color: alpha(@theme_fg_color, 0.06);
        border-color: alpha(@theme_fg_color, 0.10);
      }
      .gc-toolbar-button:active, .gc-overflow-button:active {
        background-color: alpha(@theme_selected_bg_color, 0.13);
      }
      .gc-toolbar-button:focus, .gc-overflow-button:focus {
        border-color: alpha(@theme_selected_bg_color, 0.72);
        box-shadow: 0 0 0 2px alpha(@theme_selected_bg_color, 0.20);
      }
      .gc-search-shell {
        min-height: 48px;
        margin: 0 4px;
        padding: 2px 5px 2px 14px;
        border-radius: 24px;
        background-color: alpha(@theme_base_color, 0.94);
        border: 1px solid alpha(@theme_fg_color, 0.13);
        box-shadow: 0 3px 12px alpha(#000000, 0.07);
      }
      .gc-search-entry {
        min-height: 44px;
        padding: 0 8px;
        border: none;
        box-shadow: none;
        background: transparent;
      }
      .gc-search-entry:focus { box-shadow: none; }
      .gc-search-control {
        min-width: 48px;
        min-height: 48px;
        padding: 0 9px;
        border-radius: 16px;
        border: 1px solid transparent;
        background: transparent;
        box-shadow: none;
      }
      .gc-search-control:hover {
        background-color: alpha(@theme_fg_color, 0.06);
      }
      .gc-overflow-card {
        padding: 8px;
        border-radius: 18px;
        background-color: @theme_bg_color;
        border: 1px solid alpha(@theme_fg_color, 0.12);
      }
      .gc-overflow-action {
        min-height: 44px;
        padding: 5px 12px;
        border-radius: 12px;
        background: transparent;
        border: 1px solid transparent;
        box-shadow: none;
      }
      .gc-overflow-action:hover {
        background-color: alpha(@theme_fg_color, 0.06);
      }
      .gc-internal-canvas {
        padding: 36px 20px;
        background-color: @theme_base_color;
      }
      .gc-internal-card, .gc-panel-card {
        min-width: 320px;
        padding: 30px 34px;
        border-radius: 28px;
        background-color: alpha(@theme_bg_color, 0.96);
        border: 1px solid alpha(@theme_fg_color, 0.11);
        box-shadow: 0 14px 36px alpha(#000000, 0.10);
      }
      .gc-internal-eyebrow {
        opacity: 0.70;
        font-size: 0.82em;
        font-weight: 700;
      }
      .gc-internal-title, .gc-panel-title {
        font-size: 2em;
        font-weight: 700;
      }
      .gc-internal-subtitle, .gc-panel-copy {
        opacity: 0.78;
        font-size: 1.05em;
      }
      .gc-status-row { margin-top: 10px; }
      .gc-status-chip {
        padding: 6px 10px;
        border-radius: 999px;
        background-color: alpha(@theme_fg_color, 0.05);
        border: 1px solid alpha(@theme_fg_color, 0.09);
        font-size: 0.86em;
      }
      window.gc-private-window .gc-chrome-shell {
        border-bottom-color: alpha(@theme_selected_bg_color, 0.38);
      }
      window.gc-private-window .gc-stage-badge {
        background-color: alpha(@theme_selected_bg_color, 0.15);
      }
    )CSS";
    gtk_css_provider_load_from_data(css, kCss, -1, nullptr);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
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
    add_style_class(window, "gc-browser-window");
    if (private_window) add_style_class(window, "gc-private-window");
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);

    root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root);

    chrome_shell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_style_class(chrome_shell, "gc-chrome-shell");
    gtk_box_pack_start(GTK_BOX(root), chrome_shell, FALSE, FALSE, 0);

    build_tab_strip();
    build_toolbar();

    content_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(content_stack),
                                  GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(content_stack), 120);
    gtk_box_pack_start(GTK_BOX(root), content_stack, TRUE, TRUE, 0);

    content_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(content_area, TRUE);
    gtk_widget_set_vexpand(content_area, TRUE);
    gtk_stack_add_named(GTK_STACK(content_stack), content_area, "web");
    g_signal_connect(content_area, "size-allocate",
                     G_CALLBACK(on_content_size_allocate), this);

    build_internal_surface();
    build_panel_surface();

    start_media_hover_timer();
    created = true;
    return true;
  }

  void build_tab_strip() {
    tab_strip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_style_class(tab_strip, "gc-tab-strip");
    gtk_box_pack_start(GTK_BOX(chrome_shell), tab_strip, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(tab_strip), make_brand_mark(), FALSE, FALSE, 0);

    active_tab = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_style_class(active_tab, "gc-active-tab");
    gtk_box_pack_start(GTK_BOX(tab_strip), active_tab, FALSE, FALSE, 0);

    tab_label = gtk_label_new("New Tab");
    gtk_label_set_xalign(GTK_LABEL(tab_label), 0.0F);
    gtk_label_set_ellipsize(GTK_LABEL(tab_label), PANGO_ELLIPSIZE_END);
    gtk_widget_set_size_request(tab_label, 160, -1);
    add_style_class(tab_label, "gc-tab-title");
    gtk_box_pack_start(GTK_BOX(active_tab), tab_label, TRUE, TRUE, 0);

    tab_status = gtk_label_new("");
    add_style_class(tab_status, "gc-tab-status");
    gtk_box_pack_start(GTK_BOX(active_tab), tab_status, FALSE, FALSE, 0);

    auto* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(tab_strip), spacer, TRUE, TRUE, 0);

    stage_badge = gtk_label_new(
        private_window ? "Private • Development" : "Development • Glaze 1.6");
    add_style_class(stage_badge, "gc-stage-badge");
    gtk_box_pack_end(GTK_BOX(tab_strip), stage_badge, FALSE, FALSE, 0);
  }

  void build_toolbar() {
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    add_style_class(toolbar, "gc-toolbar");
    gtk_box_pack_start(GTK_BOX(chrome_shell), toolbar, FALSE, FALSE, 0);

    add_toolbar_button(ToolbarItem::back, "←", "Back");
    add_toolbar_button(ToolbarItem::forward, "→", "Forward");
    add_toolbar_button(ToolbarItem::refresh, "↻", "Refresh or Stop");
    add_toolbar_button(ToolbarItem::home, "⌂", "Home");
    build_unified_search();
    add_toolbar_button(ToolbarItem::advanced_download_manager, "↓",
                       "Advanced Download Manager");
    build_overflow_menu();
    add_toolbar_button(ToolbarItem::settings, "⚙", "Settings");
  }

  void build_unified_search() {
    search_shell = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    add_style_class(search_shell, "gc-search-shell");
    gtk_widget_set_hexpand(search_shell, TRUE);
    set_accessible_name(search_shell, "Unified Search Bar");

    search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(
        GTK_ENTRY(search_entry),
        "Search with GoreeCloud Search or enter an address");
    gtk_widget_set_hexpand(search_entry, TRUE);
    add_style_class(search_entry, "gc-search-entry");
    set_accessible_name(search_entry, "Unified Search Bar");
    g_signal_connect(search_entry, "activate", G_CALLBACK(on_search_activate),
                     this);
    gtk_box_pack_start(GTK_BOX(search_shell), search_entry, TRUE, TRUE, 0);

    add_search_control(UnifiedSearchBarControl::advanced_reader_mode, "Aa",
                       "Advanced Reader Mode");
    add_search_control(UnifiedSearchBarControl::unified_bookmarks, "★",
                       "Unified Bookmarks");
    add_search_control(UnifiedSearchBarControl::wardveil_security, "◆",
                       "Wardveil Security");
    gtk_box_pack_start(GTK_BOX(toolbar), search_shell, TRUE, TRUE, 0);
  }

  void build_overflow_menu() {
    overflow_menu = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(overflow_menu), "⋯");
    gtk_widget_set_size_request(overflow_menu, kGlazeInteractiveTargetPx,
                                kGlazeInteractiveTargetPx);
    add_style_class(overflow_menu, "gc-overflow-button");
    set_accessible_name(overflow_menu, "Browser tools");
    gtk_box_pack_start(GTK_BOX(toolbar), overflow_menu, FALSE, FALSE, 0);

    overflow_popover = gtk_popover_new(overflow_menu);
    auto* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    add_style_class(card, "gc-overflow-card");
    gtk_container_add(GTK_CONTAINER(overflow_popover), card);

    add_overflow_action(card, ToolbarItem::privacy_shield, "Privacy Shield");
    add_overflow_action(card, ToolbarItem::wardveil_security,
                        "Wardveil Security");
    add_overflow_action(card, ToolbarItem::clipboard, "Clipboard");
    add_overflow_action(card, ToolbarItem::dns_cache, "Clear DNS Cache");
    add_overflow_action(card, ToolbarItem::advanced_proxy_manager,
                        "Advanced Proxy Manager");

    gtk_menu_button_set_popover(GTK_MENU_BUTTON(overflow_menu),
                                overflow_popover);
  }

  void add_toolbar_button(ToolbarItem item, const char* visible,
                          const char* accessible) {
    auto* button = make_toolbar_button(visible, accessible);
    toolbar_bindings.emplace(button, item);
    g_signal_connect(button, "clicked", G_CALLBACK(on_toolbar_clicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), button, FALSE, FALSE, 0);
  }

  void add_overflow_action(GtkWidget* container, ToolbarItem item,
                           const char* label) {
    auto* button = gtk_button_new_with_label(label);
    gtk_widget_set_halign(button, GTK_ALIGN_FILL);
    add_style_class(button, "gc-overflow-action");
    set_accessible_name(button, label);
    toolbar_bindings.emplace(button, item);
    g_signal_connect(button, "clicked", G_CALLBACK(on_toolbar_clicked), this);
    gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);
  }

  void add_search_control(UnifiedSearchBarControl control, const char* visible,
                          const char* accessible) {
    auto* button = gtk_button_new_with_label(visible);
    gtk_widget_set_size_request(button, kGlazeInteractiveTargetPx,
                                kGlazeInteractiveTargetPx);
    add_style_class(button, "gc-search-control");
    set_accessible_name(button, accessible);
    search_control_bindings.emplace(button, control);
    g_signal_connect(button, "clicked",
                     G_CALLBACK(on_search_control_clicked), this);
    gtk_box_pack_start(GTK_BOX(search_shell), button, FALSE, FALSE, 0);
  }

  void build_internal_surface() {
    internal_canvas = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_style_class(internal_canvas, "gc-internal-canvas");

    internal_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 11);
    gtk_widget_set_halign(internal_card, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(internal_card, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(internal_card, 320, -1);
    add_style_class(internal_card, "gc-internal-card");
    gtk_box_pack_start(GTK_BOX(internal_canvas), internal_card, TRUE, FALSE, 0);

    auto* identity_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 9);
    gtk_box_pack_start(GTK_BOX(identity_row), make_brand_mark(), FALSE, FALSE, 0);

    internal_eyebrow = gtk_label_new("GOREECLOUD BROWSER");
    gtk_label_set_xalign(GTK_LABEL(internal_eyebrow), 0.0F);
    add_style_class(internal_eyebrow, "gc-internal-eyebrow");
    gtk_box_pack_start(GTK_BOX(identity_row), internal_eyebrow, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(internal_card), identity_row, FALSE, FALSE, 0);

    internal_title = gtk_label_new("A focused place to start.");
    gtk_label_set_xalign(GTK_LABEL(internal_title), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(internal_title), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(internal_title), 48);
    add_style_class(internal_title, "gc-internal-title");
    gtk_box_pack_start(GTK_BOX(internal_card), internal_title, FALSE, FALSE, 0);

    internal_subtitle = gtk_label_new(
        "Search with GoreeCloud Search or enter an address in the navigation capsule above.");
    gtk_label_set_xalign(GTK_LABEL(internal_subtitle), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(internal_subtitle), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(internal_subtitle), 68);
    add_style_class(internal_subtitle, "gc-internal-subtitle");
    gtk_box_pack_start(GTK_BOX(internal_card), internal_subtitle, FALSE, FALSE, 0);

    auto* status_row = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(status_row), GTK_SELECTION_NONE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(status_row), 7);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(status_row), 7);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(status_row), 1);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(status_row), 3);
    add_style_class(status_row, "gc-status-row");
    gtk_flow_box_insert(GTK_FLOW_BOX(status_row),
                        make_status_chip("Development build"), -1);
    gtk_flow_box_insert(GTK_FLOW_BOX(status_row),
                        make_status_chip("Glaze UI 1.6"), -1);
    internal_status = make_status_chip("Desktop renderer integration pending");
    gtk_flow_box_insert(GTK_FLOW_BOX(status_row), internal_status, -1);
    gtk_box_pack_start(GTK_BOX(internal_card), status_row, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(content_stack), internal_canvas, "internal");
  }

  void build_panel_surface() {
    panel_canvas = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    add_style_class(panel_canvas, "gc-internal-canvas");

    panel_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_halign(panel_card, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(panel_card, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(panel_card, 320, -1);
    add_style_class(panel_card, "gc-panel-card");
    gtk_box_pack_start(GTK_BOX(panel_canvas), panel_card, TRUE, FALSE, 0);

    panel_title = gtk_label_new("Browser tool");
    gtk_label_set_xalign(GTK_LABEL(panel_title), 0.0F);
    add_style_class(panel_title, "gc-panel-title");
    gtk_box_pack_start(GTK_BOX(panel_card), panel_title, FALSE, FALSE, 0);

    panel_label = gtk_label_new(nullptr);
    gtk_label_set_xalign(GTK_LABEL(panel_label), 0.0F);
    gtk_label_set_line_wrap(GTK_LABEL(panel_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(panel_label), 72);
    add_style_class(panel_label, "gc-panel-copy");
    gtk_box_pack_start(GTK_BOX(panel_card), panel_label, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(content_stack), panel_canvas, "panel");
  }

  void set_internal_surface_copy(std::string_view url) {
    const auto copy = internal_surface_copy(url);
    if (internal_eyebrow) {
      gtk_label_set_text(GTK_LABEL(internal_eyebrow), copy.eyebrow);
    }
    if (internal_title) {
      gtk_label_set_text(GTK_LABEL(internal_title), copy.title);
    }
    if (internal_subtitle) {
      gtk_label_set_text(GTK_LABEL(internal_subtitle), copy.subtitle);
    }
    if (internal_status) {
      gtk_label_set_text(GTK_LABEL(internal_status), copy.status);
    }
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
    surface.window_handle =
        static_cast<std::uintptr_t>(gdk_x11_window_get_xid(gdk_window));
    surface.display_handle = reinterpret_cast<std::uintptr_t>(
        gdk_x11_display_get_xdisplay(display));
    surface.x = 0;
    surface.y = 0;
    surface.width = allocation.width;
    surface.height = allocation.height;
    surface.scale_factor =
        static_cast<float>(gtk_widget_get_scale_factor(content_area));
    return surface;
  }

  void attach_engine_surface() {
    if (!attached_view || !content_area ||
        !gtk_widget_get_realized(content_area)) {
      return;
    }
    auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(attached_view);
    if (!attachable) return;
    const auto surface = current_surface();
    if (surface.window_handle == 0 || surface.width <= 0 || surface.height <= 0) {
      return;
    }
    engine_surface_attached = attachable->attach_native_surface(surface);
    if (engine_surface_attached) {
      gtk_stack_set_visible_child_name(GTK_STACK(content_stack), "web");
    }
  }

  void resize_attached_engine() {
    if (!engine_surface_attached || !attached_view) return;
    auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(attached_view);
    if (!attachable) return;
    const auto surface = current_surface();
    if (surface.window_handle != 0 && surface.width > 0 && surface.height > 0) {
      attachable->resize_native_surface(surface);
    }
  }

  void update_metrics() {
    if (!window) return;
    int width = 0;
    int height = 0;
    gtk_window_get_size(GTK_WINDOW(window), &width, &height);
    metrics.width = width;
    metrics.height = height;
    metrics.scale_factor =
        static_cast<float>(gtk_widget_get_scale_factor(window));
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
  GtkWidget* chrome_shell{nullptr};
  GtkWidget* tab_strip{nullptr};
  GtkWidget* active_tab{nullptr};
  GtkWidget* tab_label{nullptr};
  GtkWidget* tab_status{nullptr};
  GtkWidget* stage_badge{nullptr};
  GtkWidget* toolbar{nullptr};
  GtkWidget* search_shell{nullptr};
  GtkWidget* search_entry{nullptr};
  GtkWidget* overflow_menu{nullptr};
  GtkWidget* overflow_popover{nullptr};
  GtkWidget* content_stack{nullptr};
  GtkWidget* content_area{nullptr};
  GtkWidget* internal_canvas{nullptr};
  GtkWidget* internal_card{nullptr};
  GtkWidget* internal_eyebrow{nullptr};
  GtkWidget* internal_title{nullptr};
  GtkWidget* internal_subtitle{nullptr};
  GtkWidget* internal_status{nullptr};
  GtkWidget* panel_canvas{nullptr};
  GtkWidget* panel_card{nullptr};
  GtkWidget* panel_title{nullptr};
  GtkWidget* panel_label{nullptr};
  GtkCssProvider* css{nullptr};

  EngineView* attached_view{nullptr};
  NativeWindowMetrics metrics{1280, 800, 1.0F};
  guint media_hover_timer_id{0};
  bool private_window{false};
  bool created{false};
  bool close_requested{false};
  bool engine_surface_attached{false};
};

GtkLinuxGlazeWindowHost::GtkLinuxGlazeWindowHost()
    : impl_(std::make_unique<Impl>()) {}

GtkLinuxGlazeWindowHost::~GtkLinuxGlazeWindowHost() {
  if (!impl_) return;
  impl_->stop_media_hover_timer();
  detach_engine_view();
  if (impl_->css) {
    g_object_unref(impl_->css);
    impl_->css = nullptr;
  }
}

void GtkLinuxGlazeWindowHost::set_toolbar_handler(ToolbarHandler handler) {
  impl_->toolbar_handler = std::move(handler);
}
void GtkLinuxGlazeWindowHost::set_search_handler(SearchHandler handler) {
  impl_->search_handler = std::move(handler);
}
void GtkLinuxGlazeWindowHost::set_search_control_handler(
    SearchControlHandler handler) {
  impl_->search_control_handler = std::move(handler);
}
void GtkLinuxGlazeWindowHost::set_media_hover_action_handler(
    MediaHoverActionHandler handler) {
  impl_->media_hover_action_handler = std::move(handler);
}
void GtkLinuxGlazeWindowHost::set_media_hover_policy(MediaHoverSitePolicy policy) {
  impl_->media_hover.set_policy(std::move(policy));
}
void GtkLinuxGlazeWindowHost::set_private_window(bool private_window) {
  impl_->private_window = private_window;
}

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
  if (impl_->window) {
    gtk_window_set_title(GTK_WINDOW(impl_->window), std::string{title}.c_str());
  }
}

void GtkLinuxGlazeWindowHost::render_chrome(const BrowserChromeState& state) {
  if (impl_->search_entry && !gtk_widget_has_focus(impl_->search_entry)) {
    gtk_entry_set_text(GTK_ENTRY(impl_->search_entry),
                       state.unified_search.display_text.c_str());
  }

  const ChromeTabPresentation* active = nullptr;
  for (const auto& tab : state.tabs) {
    if (tab.active) {
      active = &tab;
      break;
    }
  }
  if (!active && !state.tabs.empty()) active = &state.tabs.front();

  if (impl_->tab_label) {
    gtk_label_set_text(GTK_LABEL(impl_->tab_label),
                       active ? active->title.c_str() : "New Tab");
  }
  if (impl_->tab_status) {
    const char* status = "";
    if (active && active->loading) status = "Loading";
    else if (active && active->private_context) status = "Private";
    gtk_label_set_text(GTK_LABEL(impl_->tab_status), status);
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
    if (auto* attachable =
            dynamic_cast<NativeSurfaceAttachable*>(impl_->attached_view)) {
      if (attachable->native_surface_attached()) {
        attachable->detach_native_surface();
      }
    }
  }
  impl_->attached_view = nullptr;
  impl_->engine_surface_attached = false;
}

void GtkLinuxGlazeWindowHost::show_internal_surface(
    std::string_view internal_url) {
  impl_->media_hover.invalidate();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (!impl_->content_stack || !impl_->internal_canvas) return;
  impl_->set_internal_surface_copy(internal_url);
  gtk_stack_set_visible_child_name(GTK_STACK(impl_->content_stack), "internal");
}

void GtkLinuxGlazeWindowHost::show_panel(std::string_view panel_id) {
  impl_->media_hover.invalidate();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (!impl_->content_stack || !impl_->panel_label) return;
  gtk_label_set_text(GTK_LABEL(impl_->panel_label), std::string{panel_id}.c_str());
  gtk_stack_set_visible_child_name(GTK_STACK(impl_->content_stack), "panel");
}

NativeWindowMetrics GtkLinuxGlazeWindowHost::metrics() const {
  return impl_->metrics;
}

bool GtkLinuxGlazeWindowHost::pump_events() {
  while (gtk_events_pending()) gtk_main_iteration_do(FALSE);
  return !impl_->close_requested;
}

bool GtkLinuxGlazeWindowHost::close_requested() const noexcept {
  return impl_->close_requested;
}

}  // namespace goreecloud::browser::platform
