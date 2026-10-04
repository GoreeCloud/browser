#include "goreecloud/browser/platform/gtk_linux_glaze_host.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "goreecloud/browser/glaze.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/live_media_hover_coordinator.hpp"
#include "goreecloud/browser/media_hit_test_provider.hpp"
#include "goreecloud/browser/native_engine_surface.hpp"
#include "goreecloud/browser/panel_surface.hpp"
#include "goreecloud/browser/platform/gtk_media_hover_popover.hpp"

namespace goreecloud::browser::platform {
namespace {

inline constexpr int kGlazeInteractiveTargetPx =
    static_cast<int>(kBrowserGlazeCapabilities.minimum_target_px);

bool environment_flag_enabled(const char* name) {
  const char* value = std::getenv(name);
  return value && *value && std::string_view{value} != "0";
}

void add_style_class(GtkWidget* widget, const char* class_name) {
  gtk_style_context_add_class(gtk_widget_get_style_context(widget), class_name);
}

void set_accessible_name(GtkWidget* widget, const char* name) {
  gtk_widget_set_tooltip_text(widget, name);
  if (auto* accessible = gtk_widget_get_accessible(widget)) {
    atk_object_set_name(accessible, name);
  }
}

void set_button_icon(GtkWidget* button, const char* fallback_label,
                     const char* icon_name) {
  auto* theme = gtk_icon_theme_get_default();
  if (theme && icon_name && gtk_icon_theme_has_icon(theme, icon_name)) {
    auto* image = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    gtk_image_set_pixel_size(GTK_IMAGE(image), 20);
    gtk_button_set_image(GTK_BUTTON(button), image);
    gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
  } else {
    gtk_button_set_label(GTK_BUTTON(button), fallback_label);
  }
}

GtkWidget* make_toolbar_button(const char* fallback_label,
                               const char* icon_name,
                               const char* accessible_name) {
  auto* button = gtk_button_new();
  set_button_icon(button, fallback_label, icon_name);
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
            "Development browser surface"};
  }
  if (url == kHomeUrl) {
    return {"GOREECLOUD HOME", "Your browser, your workspace.",
            "Home is a first-party GoreeCloud surface. Rich modules and synchronized content remain development work.",
            "Development surface"};
  }
  if (url == kSettingsUrl) {
    return {"BROWSER SETTINGS", "Control the browser without losing context.",
            "Browse the Browser-owned settings areas below. Functional controls appear only as their owning runtime and provider integrations become available.",
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

class GtkLinuxGlazeWindowHost::Impl : public NativeSurfaceFrameSink,
                                      public NativeSurfaceCursorSink,
                                      public NativeSurfaceContextMenuSink {
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
    self->dismiss_active_context_menu();
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

  static void on_tab_activate_clicked(GtkButton* button, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    const auto* id =
        static_cast<const char*>(g_object_get_data(G_OBJECT(button), "gc-tab-id"));
    if (self->tab_action_handler && id) {
      self->tab_action_handler(GtkTabAction::activate, id);
    }
  }

  static void on_tab_close_clicked(GtkButton* button, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    const auto* id =
        static_cast<const char*>(g_object_get_data(G_OBJECT(button), "gc-tab-id"));
    if (self->tab_action_handler && id) {
      self->tab_action_handler(GtkTabAction::close, id);
    }
  }

  static void on_new_tab_clicked(GtkButton*, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (self->tab_action_handler) {
      self->tab_action_handler(GtkTabAction::create, {});
    }
  }

  static gboolean on_window_key_press(GtkWidget*, GdkEventKey* event,
                                      gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!event) return FALSE;

    const bool control = (event->state & GDK_CONTROL_MASK) != 0;
    const bool shift = (event->state & GDK_SHIFT_MASK) != 0;
    const bool alt = (event->state & GDK_MOD1_MASK) != 0;

    if (((control && (event->keyval == GDK_KEY_l ||
                       event->keyval == GDK_KEY_k)) ||
         event->keyval == GDK_KEY_F6) &&
        self->search_entry) {
      gtk_widget_grab_focus(self->search_entry);
      gtk_editable_select_region(GTK_EDITABLE(self->search_entry), 0, -1);
      return TRUE;
    }
    if (control && !shift && event->keyval == GDK_KEY_t &&
        self->tab_action_handler) {
      self->tab_action_handler(GtkTabAction::create, {});
      return TRUE;
    }
    if (control && !shift && event->keyval == GDK_KEY_w &&
        self->tab_action_handler && !self->active_tab_id.empty()) {
      self->tab_action_handler(GtkTabAction::close, self->active_tab_id);
      return TRUE;
    }
    if (control &&
        (event->keyval == GDK_KEY_Tab || event->keyval == GDK_KEY_Page_Down) &&
        self->tab_action_handler) {
      self->tab_action_handler(shift ? GtkTabAction::previous
                                     : GtkTabAction::next,
                               {});
      return TRUE;
    }
    if (control &&
        (event->keyval == GDK_KEY_ISO_Left_Tab ||
         event->keyval == GDK_KEY_Page_Up) &&
        self->tab_action_handler) {
      self->tab_action_handler(GtkTabAction::previous, {});
      return TRUE;
    }
    if ((control && event->keyval == GDK_KEY_r) ||
        event->keyval == GDK_KEY_F5) {
      if (self->toolbar_handler) self->toolbar_handler(ToolbarItem::refresh);
      return TRUE;
    }
    if (alt && event->keyval == GDK_KEY_Left) {
      if (self->toolbar_handler) self->toolbar_handler(ToolbarItem::back);
      return TRUE;
    }
    if (alt && event->keyval == GDK_KEY_Right) {
      if (self->toolbar_handler) self->toolbar_handler(ToolbarItem::forward);
      return TRUE;
    }
    if (alt && event->keyval == GDK_KEY_Home) {
      if (self->toolbar_handler) self->toolbar_handler(ToolbarItem::home);
      return TRUE;
    }
    if (event->keyval == GDK_KEY_Escape && self->panel_is_visible()) {
      self->close_panel();
      return TRUE;
    }
    return FALSE;
  }

  static void on_search_activate(GtkEntry* entry, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!self->search_handler) return;
    const char* value = gtk_entry_get_text(entry);
    self->search_handler(value ? std::string_view{value} : std::string_view{});
  }

  static void on_panel_close_clicked(GtkButton*, gpointer data) {
    static_cast<Impl*>(data)->close_panel();
  }

  static void on_content_size_allocate(GtkWidget*, GtkAllocation*,
                                       gpointer data) {
    auto* self = static_cast<Impl*>(data);
    self->update_metrics();
    self->resize_attached_engine();
    self->media_hover.invalidate();
    if (self->content_area) hide_gtk_media_hover_popover(self->content_area);
  }

  static gboolean on_content_draw(GtkWidget* widget, cairo_t* cr,
                                  gpointer data) {
    return static_cast<Impl*>(data)->draw_software_frame(widget, cr);
  }

  static std::uint32_t input_modifiers(guint state) {
    std::uint32_t modifiers = native_modifier_none;
    if (state & GDK_SHIFT_MASK) modifiers |= native_modifier_shift;
    if (state & GDK_CONTROL_MASK) modifiers |= native_modifier_control;
    if (state & GDK_MOD1_MASK) modifiers |= native_modifier_alt;
    if (state & GDK_LOCK_MASK) modifiers |= native_modifier_caps_lock;
    if (state & GDK_BUTTON1_MASK) modifiers |= native_modifier_left_button;
    if (state & GDK_BUTTON2_MASK) modifiers |= native_modifier_middle_button;
    if (state & GDK_BUTTON3_MASK) modifiers |= native_modifier_right_button;
    return modifiers;
  }

  static int virtual_key_code(guint keyval) {
    const auto upper = gdk_keyval_to_upper(keyval);
    if (upper >= GDK_KEY_A && upper <= GDK_KEY_Z) {
      return static_cast<int>(upper);
    }
    if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
      return static_cast<int>(keyval);
    }

    switch (keyval) {
      case GDK_KEY_BackSpace: return 0x08;
      case GDK_KEY_Tab:
      case GDK_KEY_ISO_Left_Tab: return 0x09;
      case GDK_KEY_Return:
      case GDK_KEY_KP_Enter:
      case GDK_KEY_ISO_Enter: return 0x0D;
      case GDK_KEY_Shift_L:
      case GDK_KEY_Shift_R: return 0x10;
      case GDK_KEY_Caps_Lock: return 0x14;
      case GDK_KEY_Escape: return 0x1B;
      case GDK_KEY_space:
      case GDK_KEY_KP_Space: return 0x20;
      case GDK_KEY_Page_Up:
      case GDK_KEY_KP_Page_Up: return 0x21;
      case GDK_KEY_Page_Down:
      case GDK_KEY_KP_Page_Down: return 0x22;
      case GDK_KEY_End:
      case GDK_KEY_KP_End: return 0x23;
      case GDK_KEY_Home:
      case GDK_KEY_KP_Home: return 0x24;
      case GDK_KEY_Left:
      case GDK_KEY_KP_Left: return 0x25;
      case GDK_KEY_Up:
      case GDK_KEY_KP_Up: return 0x26;
      case GDK_KEY_Right:
      case GDK_KEY_KP_Right: return 0x27;
      case GDK_KEY_Down:
      case GDK_KEY_KP_Down: return 0x28;
      case GDK_KEY_Insert:
      case GDK_KEY_KP_Insert: return 0x2D;
      case GDK_KEY_Delete:
      case GDK_KEY_KP_Delete: return 0x2E;
      case GDK_KEY_exclam: return 0x31;
      case GDK_KEY_at: return 0x32;
      case GDK_KEY_numbersign: return 0x33;
      case GDK_KEY_dollar: return 0x34;
      case GDK_KEY_percent: return 0x35;
      case GDK_KEY_asciicircum: return 0x36;
      case GDK_KEY_ampersand: return 0x37;
      case GDK_KEY_asterisk: return 0x38;
      case GDK_KEY_parenleft: return 0x39;
      case GDK_KEY_parenright: return 0x30;
      case GDK_KEY_semicolon:
      case GDK_KEY_colon: return 0xBA;
      case GDK_KEY_equal:
      case GDK_KEY_plus: return 0xBB;
      case GDK_KEY_comma:
      case GDK_KEY_less: return 0xBC;
      case GDK_KEY_minus:
      case GDK_KEY_underscore: return 0xBD;
      case GDK_KEY_period:
      case GDK_KEY_greater: return 0xBE;
      case GDK_KEY_slash:
      case GDK_KEY_question: return 0xBF;
      case GDK_KEY_grave:
      case GDK_KEY_asciitilde: return 0xC0;
      case GDK_KEY_bracketleft:
      case GDK_KEY_braceleft: return 0xDB;
      case GDK_KEY_backslash:
      case GDK_KEY_bar: return 0xDC;
      case GDK_KEY_bracketright:
      case GDK_KEY_braceright: return 0xDD;
      case GDK_KEY_apostrophe:
      case GDK_KEY_quotedbl: return 0xDE;
      default:
        break;
    }

    if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F24) {
      return 0x70 + static_cast<int>(keyval - GDK_KEY_F1);
    }
    if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9) {
      return 0x60 + static_cast<int>(keyval - GDK_KEY_KP_0);
    }
    switch (keyval) {
      case GDK_KEY_KP_Multiply: return 0x6A;
      case GDK_KEY_KP_Add: return 0x6B;
      case GDK_KEY_KP_Separator: return 0x6C;
      case GDK_KEY_KP_Subtract: return 0x6D;
      case GDK_KEY_KP_Decimal: return 0x6E;
      case GDK_KEY_KP_Divide: return 0x6F;
      default:
        return 0;
    }
  }

  static std::uint32_t direct_character(guint keyval) {
    const auto character = gdk_keyval_to_unicode(keyval);
    if (character == 0 || character > 0xFFFFU ||
        !g_unichar_isprint(character)) {
      return 0;
    }
    return static_cast<std::uint32_t>(character);
  }

  static bool defer_key_to_browser_chrome(const GdkEventKey& event) {
    const bool control = (event.state & GDK_CONTROL_MASK) != 0;
    const bool shift = (event.state & GDK_SHIFT_MASK) != 0;
    const bool alt = (event.state & GDK_MOD1_MASK) != 0;

    // Keep system-style modifiers reserved until their Browser/page ownership
    // contract is explicit. This tranche forwards only non-conflicting Control
    // combinations after preserving the shortcuts owned by Browser chrome.
    if (alt || (event.state & (GDK_SUPER_MASK | GDK_META_MASK))) return true;
    if (event.keyval == GDK_KEY_F5 || event.keyval == GDK_KEY_F6) return true;
    if (!control) return false;

    if (event.keyval == GDK_KEY_l || event.keyval == GDK_KEY_k ||
        event.keyval == GDK_KEY_r || event.keyval == GDK_KEY_Tab ||
        event.keyval == GDK_KEY_Page_Down ||
        event.keyval == GDK_KEY_ISO_Left_Tab ||
        event.keyval == GDK_KEY_Page_Up) {
      return true;
    }
    if (!shift &&
        (event.keyval == GDK_KEY_t || event.keyval == GDK_KEY_w)) {
      return true;
    }
    return false;
  }

  NativeSurfaceInputForwarder* software_input_forwarder() {
    if (!software_surface_attached || !attached_view) return nullptr;
    return dynamic_cast<NativeSurfaceInputForwarder*>(attached_view);
  }

  NativeSurfaceTextInputForwarder* software_text_input_forwarder() {
    if (!software_surface_attached || !attached_view) return nullptr;
    return dynamic_cast<NativeSurfaceTextInputForwarder*>(attached_view);
  }

  static std::optional<std::u16string> utf8_to_utf16(const char* text,
                                                      gssize length = -1) {
    if (!text) return std::u16string{};
    GError* error = nullptr;
    glong items_written = 0;
    gunichar2* converted =
        g_utf8_to_utf16(text, length, nullptr, &items_written, &error);
    if (!converted) {
      if (error) g_error_free(error);
      return std::nullopt;
    }

    std::u16string result;
    result.reserve(static_cast<std::size_t>(items_written));
    for (glong i = 0; i < items_written; ++i) {
      result.push_back(static_cast<char16_t>(converted[i]));
    }
    g_free(converted);
    if (error) g_error_free(error);
    return result;
  }

  static int utf16_cursor_position(const char* text, int character_offset) {
    if (!text || character_offset <= 0) return 0;
    const char* cursor = g_utf8_offset_to_pointer(text, character_offset);
    const auto prefix = utf8_to_utf16(text, cursor - text);
    return prefix ? static_cast<int>(prefix->size()) : 0;
  }

  void reset_text_input() {
    if (auto* forwarder = software_text_input_forwarder()) {
      forwarder->cancel_text_composition();
    }
    ime_preedit_active = false;
    if (im_context) gtk_im_context_reset(im_context);
  }

  static NativePopupRect clamp_popup_rect(NativePopupRect rect,
                                           int view_width,
                                           int view_height) {
    if (rect.width <= 0 || rect.height <= 0 ||
        view_width <= 0 || view_height <= 0) {
      return {};
    }
    if (rect.x < 0) rect.x = 0;
    if (rect.y < 0) rect.y = 0;
    if (rect.x + rect.width > view_width) rect.x = view_width - rect.width;
    if (rect.y + rect.height > view_height) rect.y = view_height - rect.height;
    if (rect.x < 0) rect.x = 0;
    if (rect.y < 0) rect.y = 0;
    return rect;
  }

  NativePointerEvent pointer_event(double x, double y, guint state) {
    int pointer_x = static_cast<int>(x);
    int pointer_y = static_cast<int>(y);

    GtkAllocation allocation{};
    if (content_area) gtk_widget_get_allocation(content_area, &allocation);

    {
      std::scoped_lock lock(software_frame_mutex);
      if (!software_popup_frame.empty() &&
          software_popup_rect.width > 0 &&
          software_popup_rect.height > 0) {
        const auto displayed =
            clamp_popup_rect(software_popup_rect, allocation.width,
                             allocation.height);
        if (pointer_x >= displayed.x &&
            pointer_x < displayed.x + displayed.width &&
            pointer_y >= displayed.y &&
            pointer_y < displayed.y + displayed.height) {
          pointer_x += software_popup_rect.x - displayed.x;
          pointer_y += software_popup_rect.y - displayed.y;
        }
      }
    }

    return NativePointerEvent{.x = pointer_x,
                              .y = pointer_y,
                              .modifiers = input_modifiers(state)};
  }

  static gboolean on_content_motion(GtkWidget*, GdkEventMotion* event,
                                    gpointer data) {
    auto* self = static_cast<Impl*>(data);
    auto* forwarder = self->software_input_forwarder();
    if (!forwarder || !event) return FALSE;
    return forwarder->send_pointer_move(
               self->pointer_event(event->x, event->y, event->state), false)
               ? TRUE
               : FALSE;
  }

  static gboolean on_content_enter(GtkWidget*, GdkEventCrossing* event,
                                   gpointer data) {
    auto* self = static_cast<Impl*>(data);
    auto* forwarder = self->software_input_forwarder();
    if (!forwarder || !event) return FALSE;
    return forwarder->send_pointer_move(
               self->pointer_event(event->x, event->y, event->state), false)
               ? TRUE
               : FALSE;
  }

  static gboolean on_content_leave(GtkWidget*, GdkEventCrossing* event,
                                   gpointer data) {
    auto* self = static_cast<Impl*>(data);
    auto* forwarder = self->software_input_forwarder();
    if (!forwarder || !event) return FALSE;
    return forwarder->send_pointer_move(
               self->pointer_event(event->x, event->y, event->state), true)
               ? TRUE
               : FALSE;
  }

  static gboolean on_content_button(GtkWidget* widget, GdkEventButton* event,
                                    gpointer data) {
    auto* self = static_cast<Impl*>(data);
    auto* forwarder = self->software_input_forwarder();
    if (!forwarder || !event) return FALSE;

    NativePointerButton button;
    if (event->button == 1) button = NativePointerButton::left;
    else if (event->button == 2) button = NativePointerButton::middle;
    else if (event->button == 3) button = NativePointerButton::right;
    else return FALSE;

    int click_count = 1;
    if (event->type == GDK_2BUTTON_PRESS) click_count = 2;
    else if (event->type == GDK_3BUTTON_PRESS) click_count = 3;

    const bool pressed = event->type != GDK_BUTTON_RELEASE;
    if (pressed) gtk_widget_grab_focus(widget);
    return forwarder->send_pointer_button(
               self->pointer_event(event->x, event->y, event->state),
               button, pressed, click_count)
               ? TRUE
               : FALSE;
  }

  static gboolean on_content_scroll(GtkWidget*, GdkEventScroll* event,
                                    gpointer data) {
    auto* self = static_cast<Impl*>(data);
    auto* forwarder = self->software_input_forwarder();
    if (!forwarder || !event) return FALSE;

    int delta_x = 0;
    int delta_y = 0;
    switch (event->direction) {
      case GDK_SCROLL_UP:
        delta_y = 120;
        break;
      case GDK_SCROLL_DOWN:
        delta_y = -120;
        break;
      case GDK_SCROLL_LEFT:
        delta_x = -120;
        break;
      case GDK_SCROLL_RIGHT:
        delta_x = 120;
        break;
      case GDK_SCROLL_SMOOTH:
        delta_x = static_cast<int>(-event->delta_x * 120.0);
        delta_y = static_cast<int>(-event->delta_y * 120.0);
        break;
      default:
        break;
    }
    if (delta_x == 0 && delta_y == 0) return FALSE;
    return forwarder->send_pointer_wheel(
               self->pointer_event(event->x, event->y, event->state),
               delta_x, delta_y)
               ? TRUE
               : FALSE;
  }

  static void on_im_commit(GtkIMContext*, gchar* text, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!self || !text || !*text) return;
    const auto converted = utf8_to_utf16(text);
    if (!converted) return;

    if (auto* forwarder = self->software_text_input_forwarder()) {
      if (forwarder->commit_text(*converted)) {
        self->ime_preedit_active = false;
        if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
          std::cerr << "[GoreeCloud GTK] windowless-ime-commit length="
                    << converted->size() << std::endl;
        }
      }
    }
  }

  static void on_im_preedit_changed(GtkIMContext* context, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (!self || !context) return;

    gchar* text = nullptr;
    PangoAttrList* attributes = nullptr;
    gint cursor_position = 0;
    gtk_im_context_get_preedit_string(
        context, &text, &attributes, &cursor_position);

    const auto converted = utf8_to_utf16(text ? text : "");
    if (attributes) pango_attr_list_unref(attributes);
    if (!converted) {
      g_free(text);
      return;
    }

    auto* forwarder = self->software_text_input_forwarder();
    if (!forwarder) {
      g_free(text);
      return;
    }

    if (converted->empty()) {
      if (self->ime_preedit_active) {
        forwarder->cancel_text_composition();
      }
      self->ime_preedit_active = false;
      g_free(text);
      return;
    }

    const int selection =
        utf16_cursor_position(text ? text : "", cursor_position);
    if (forwarder->set_text_composition(*converted, selection, selection)) {
      self->ime_preedit_active = true;
      if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
        std::cerr << "[GoreeCloud GTK] windowless-ime-preedit length="
                  << converted->size() << " cursor=" << selection << std::endl;
      }
    }
    g_free(text);
  }

  static void on_im_preedit_end(GtkIMContext*, gpointer data) {
    auto* self = static_cast<Impl*>(data);
    if (self) self->ime_preedit_active = false;
  }

  static gboolean on_content_key(GtkWidget*, GdkEventKey* event,
                                  gpointer data) {
    auto* self = static_cast<Impl*>(data);
    auto* forwarder = self->software_input_forwarder();
    if (!forwarder || !event || defer_key_to_browser_chrome(*event)) {
      return FALSE;
    }

    if (self->im_context &&
        !environment_flag_enabled("GOREECLOUD_BROWSER_DISABLE_GTK_IME") &&
        gtk_im_context_filter_keypress(self->im_context, event)) {
      return TRUE;
    }

    const int vkey = virtual_key_code(event->keyval);
    const auto modifiers = input_modifiers(event->state);
    const auto character = direct_character(event->keyval);
    const bool command_modified =
        (modifiers & (native_modifier_control | native_modifier_alt)) != 0;
    bool handled = false;

    if (event->type == GDK_KEY_PRESS) {
      if (vkey > 0) {
        handled |= forwarder->send_key_event(
            NativeKeyEvent{.type = NativeKeyEventType::raw_key_down,
                           .virtual_key_code = vkey,
                           .native_key_code =
                               static_cast<int>(event->hardware_keycode),
                           .modifiers = modifiers,
                           .character = character,
                           .unmodified_character = character});
      }
      if (character != 0 && !command_modified) {
        handled |= forwarder->send_key_event(
            NativeKeyEvent{.type = NativeKeyEventType::character,
                           .virtual_key_code = vkey,
                           .native_key_code =
                               static_cast<int>(event->hardware_keycode),
                           .modifiers = modifiers,
                           .character = character,
                           .unmodified_character = character});
      }
    } else if (event->type == GDK_KEY_RELEASE && vkey > 0) {
      handled |= forwarder->send_key_event(
          NativeKeyEvent{.type = NativeKeyEventType::key_up,
                         .virtual_key_code = vkey,
                         .native_key_code =
                             static_cast<int>(event->hardware_keycode),
                         .modifiers = modifiers,
                         .character = character,
                         .unmodified_character = character});
    }

    return handled ? TRUE : FALSE;
  }

  static gboolean on_content_focus(GtkWidget* widget, GdkEventFocus* event,
                                   gpointer data) {
    auto* self = static_cast<Impl*>(data);
    const bool focused = event && event->in;
    if (auto* forwarder = self->software_input_forwarder()) {
      forwarder->set_surface_focus(focused);
    }
    if (self->im_context) {
      if (focused) {
        gtk_im_context_set_client_window(
            self->im_context, widget ? gtk_widget_get_window(widget) : nullptr);
        gtk_im_context_focus_in(self->im_context);
      } else {
        gtk_im_context_focus_out(self->im_context);
        self->reset_text_input();
      }
    }
    return FALSE;
  }

  struct ContextMenuSession {
    Impl* owner{nullptr};
    NativeContextMenuSelectionCallback callback;
    GtkWidget* popover{nullptr};
    bool completed{false};
  };

  static void finish_context_menu(
      ContextMenuSession* session,
      std::optional<int> command_id) {
    if (!session || session->completed) return;
    session->completed = true;
    if (session->callback) session->callback(command_id);
  }

  void dismiss_active_context_menu() {
    if (!active_context_popover) return;
    auto* popover = active_context_popover;
    g_object_ref(popover);
    active_context_popover = nullptr;
    auto* session = static_cast<ContextMenuSession*>(
        g_object_get_data(G_OBJECT(popover), "gc-context-menu-session"));
    finish_context_menu(session, std::nullopt);
    if (!gtk_widget_in_destruction(popover)) gtk_widget_destroy(popover);
    g_object_unref(popover);
  }

  static void on_context_menu_row_clicked(GtkButton* button, gpointer data) {
    auto* session = static_cast<ContextMenuSession*>(data);
    auto* stored = static_cast<int*>(
        g_object_get_data(G_OBJECT(button), "gc-context-command"));
    auto* popover = session ? session->popover : nullptr;
    if (popover) g_object_ref(popover);
    finish_context_menu(
        session, stored ? std::optional<int>{*stored} : std::nullopt);
    if (popover) {
      if (!gtk_widget_in_destruction(popover)) {
        gtk_popover_popdown(GTK_POPOVER(popover));
      }
      g_object_unref(popover);
    }
  }

  static void on_context_popover_closed(GtkPopover* popover, gpointer data) {
    auto* session = static_cast<ContextMenuSession*>(data);
    if (session && session->owner &&
        session->owner->active_context_popover == GTK_WIDGET(popover)) {
      session->owner->active_context_popover = nullptr;
    }
    finish_context_menu(session, std::nullopt);
    if (!gtk_widget_in_destruction(GTK_WIDGET(popover))) {
      gtk_widget_destroy(GTK_WIDGET(popover));
    }
  }

  static gboolean on_context_menu_stability_check(gpointer data) {
    auto* popover = GTK_WIDGET(data);
    const bool visible = popover && gtk_widget_get_visible(popover);
    if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
      std::cerr << "[GoreeCloud GTK] windowless-context-menu-stable visible="
                << (visible ? "yes" : "no") << std::endl;
    }
    if (popover) g_object_unref(popover);
    return G_SOURCE_REMOVE;
  }

  static GtkWidget* build_context_popover_box(
      const std::vector<NativeContextMenuItem>& items,
      ContextMenuSession* session) {
    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    add_style_class(box, "gc-context-popover-box");

    for (const auto& item : items) {
      if (item.type == NativeContextMenuItemType::separator) {
        auto* separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        add_style_class(separator, "gc-context-separator");
        gtk_box_pack_start(GTK_BOX(box), separator, FALSE, FALSE, 3);
        continue;
      }

      GtkWidget* row = nullptr;
      if (item.type == NativeContextMenuItemType::submenu) {
        auto* menu_button = gtk_menu_button_new();
        auto* label = gtk_label_new(item.label.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
        gtk_container_add(GTK_CONTAINER(menu_button), label);
        auto* submenu = gtk_popover_new(menu_button);
        auto* submenu_box = build_context_popover_box(item.children, session);
        gtk_container_add(GTK_CONTAINER(submenu), submenu_box);
        gtk_popover_set_position(GTK_POPOVER(submenu), GTK_POS_RIGHT);
        gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), submenu);
        row = menu_button;
      } else {
        std::string label = item.label;
        if ((item.type == NativeContextMenuItemType::check ||
             item.type == NativeContextMenuItemType::radio) &&
            item.checked) {
          label = "✓ " + label;
        }
        auto* button = gtk_button_new_with_label(label.c_str());
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        gtk_widget_set_halign(button, GTK_ALIGN_FILL);
        g_object_set_data_full(
            G_OBJECT(button), "gc-context-command",
            new int(item.command_id),
            [](gpointer value) { delete static_cast<int*>(value); });
        g_signal_connect(button, "clicked",
                         G_CALLBACK(on_context_menu_row_clicked), session);
        row = button;
      }

      if (!row) continue;
      gtk_widget_set_sensitive(row, item.enabled ? TRUE : FALSE);
      add_style_class(row, "gc-context-row");
      gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    }

    return box;
  }

  static const char* native_cursor_debug_name(NativeCursorType cursor) {
    switch (cursor) {
      case NativeCursorType::pointer: return "pointer";
      case NativeCursorType::crosshair: return "crosshair";
      case NativeCursorType::hand: return "hand";
      case NativeCursorType::text: return "text";
      case NativeCursorType::wait: return "wait";
      case NativeCursorType::help: return "help";
      case NativeCursorType::move: return "move";
      case NativeCursorType::east_west_resize: return "east-west-resize";
      case NativeCursorType::north_south_resize: return "north-south-resize";
      case NativeCursorType::northeast_southwest_resize:
        return "northeast-southwest-resize";
      case NativeCursorType::northwest_southeast_resize:
        return "northwest-southeast-resize";
      case NativeCursorType::column_resize: return "column-resize";
      case NativeCursorType::row_resize: return "row-resize";
      case NativeCursorType::vertical_text: return "vertical-text";
      case NativeCursorType::cell: return "cell";
      case NativeCursorType::context_menu: return "context-menu";
      case NativeCursorType::alias: return "alias";
      case NativeCursorType::progress: return "progress";
      case NativeCursorType::no_drop: return "no-drop";
      case NativeCursorType::copy: return "copy";
      case NativeCursorType::none: return "none";
      case NativeCursorType::not_allowed: return "not-allowed";
      case NativeCursorType::zoom_in: return "zoom-in";
      case NativeCursorType::zoom_out: return "zoom-out";
      case NativeCursorType::grab: return "grab";
      case NativeCursorType::grabbing: return "grabbing";
    }
    return "pointer";
  }

  static const char* gtk_cursor_name(NativeCursorType cursor) {
    switch (cursor) {
      case NativeCursorType::crosshair: return "crosshair";
      case NativeCursorType::hand: return "pointer";
      case NativeCursorType::text: return "text";
      case NativeCursorType::wait: return "wait";
      case NativeCursorType::help: return "help";
      case NativeCursorType::move: return "move";
      case NativeCursorType::east_west_resize: return "ew-resize";
      case NativeCursorType::north_south_resize: return "ns-resize";
      case NativeCursorType::northeast_southwest_resize: return "nesw-resize";
      case NativeCursorType::northwest_southeast_resize: return "nwse-resize";
      case NativeCursorType::column_resize: return "col-resize";
      case NativeCursorType::row_resize: return "row-resize";
      case NativeCursorType::vertical_text: return "vertical-text";
      case NativeCursorType::cell: return "cell";
      case NativeCursorType::context_menu: return "context-menu";
      case NativeCursorType::alias: return "alias";
      case NativeCursorType::progress: return "progress";
      case NativeCursorType::no_drop: return "no-drop";
      case NativeCursorType::copy: return "copy";
      case NativeCursorType::not_allowed: return "not-allowed";
      case NativeCursorType::zoom_in: return "zoom-in";
      case NativeCursorType::zoom_out: return "zoom-out";
      case NativeCursorType::grab: return "grab";
      case NativeCursorType::grabbing: return "grabbing";
      case NativeCursorType::pointer:
      case NativeCursorType::none:
        return nullptr;
    }
    return nullptr;
  }

  void reset_native_cursor() {
    active_cursor.reset();
    if (!content_area || !gtk_widget_get_realized(content_area)) return;
    if (auto* window = gtk_widget_get_window(content_area)) {
      gdk_window_set_cursor(window, nullptr);
    }
  }

  void apply_native_cursor(NativeCursorType cursor) override {
    if (active_cursor && *active_cursor == cursor) return;
    if (!content_area || !gtk_widget_get_realized(content_area)) return;

    auto* window = gtk_widget_get_window(content_area);
    if (!window) return;
    auto* display = gdk_window_get_display(window);
    if (!display) return;

    GdkCursor* native_cursor = nullptr;
    if (cursor == NativeCursorType::none) {
      native_cursor = gdk_cursor_new_for_display(display, GDK_BLANK_CURSOR);
    } else if (cursor != NativeCursorType::pointer) {
      if (const char* name = gtk_cursor_name(cursor)) {
        native_cursor = gdk_cursor_new_from_name(display, name);
      }
      if (!native_cursor) {
        native_cursor = gdk_cursor_new_for_display(display, GDK_LEFT_PTR);
      }
    }

    gdk_window_set_cursor(window, native_cursor);
    if (native_cursor) g_object_unref(native_cursor);
    active_cursor = cursor;

    if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
      std::cerr << "[GoreeCloud GTK] windowless-cursor-applied type="
                << native_cursor_debug_name(cursor) << std::endl;
    }
  }

  void apply_custom_cursor(NativeCustomCursor cursor) override {
    active_cursor.reset();
    if (!content_area || !gtk_widget_get_realized(content_area)) return;

    auto* window = gtk_widget_get_window(content_area);
    if (!window) return;
    auto* display = gdk_window_get_display(window);
    if (!display) return;

    const auto expected =
        static_cast<std::size_t>(cursor.width) *
        static_cast<std::size_t>(cursor.height) * 4U;
    if (cursor.width <= 0 || cursor.height <= 0 ||
        cursor.bgra.size() != expected) {
      reset_native_cursor();
      if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
        std::cerr << "[GoreeCloud GTK] windowless-custom-cursor-rejected size="
                  << cursor.width << "x" << cursor.height << std::endl;
      }
      return;
    }

    auto* pixbuf = gdk_pixbuf_new(
        GDK_COLORSPACE_RGB, TRUE, 8, cursor.width, cursor.height);
    if (!pixbuf) {
      reset_native_cursor();
      return;
    }

    auto* pixels = gdk_pixbuf_get_pixels(pixbuf);
    const int row_stride = gdk_pixbuf_get_rowstride(pixbuf);
    for (int y = 0; y < cursor.height; ++y) {
      auto* output_row = pixels + static_cast<std::size_t>(y) * row_stride;
      const auto* input_row =
          cursor.bgra.data() +
          static_cast<std::size_t>(y) * cursor.width * 4U;
      for (int x = 0; x < cursor.width; ++x) {
        const auto* input = input_row + static_cast<std::size_t>(x) * 4U;
        auto* output = output_row + static_cast<std::size_t>(x) * 4U;
        output[0] = input[2];
        output[1] = input[1];
        output[2] = input[0];
        output[3] = input[3];
      }
    }

    auto* native_cursor = gdk_cursor_new_from_pixbuf(
        display, pixbuf, cursor.hotspot_x, cursor.hotspot_y);
    g_object_unref(pixbuf);
    if (!native_cursor) {
      reset_native_cursor();
      return;
    }

    gdk_window_set_cursor(window, native_cursor);
    g_object_unref(native_cursor);

    if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
      std::cerr << "[GoreeCloud GTK] windowless-custom-cursor-applied size="
                << cursor.width << "x" << cursor.height
                << " hotspot=" << cursor.hotspot_x << "," << cursor.hotspot_y
                << " scale=" << cursor.scale_factor << std::endl;
    }
  }

  void show_native_context_menu(
      NativeContextMenuRequest request,
      NativeContextMenuSelectionCallback callback) override {
    if (!content_area || !gtk_widget_get_realized(content_area) ||
        request.items.empty()) {
      if (callback) callback(std::nullopt);
      return;
    }

    dismiss_active_context_menu();

    auto* popover = gtk_popover_new(content_area);
    add_style_class(popover, "gc-context-popover");
    gtk_popover_set_modal(GTK_POPOVER(popover), TRUE);

    auto* session =
        new ContextMenuSession{this, std::move(callback), popover, false};
    g_object_set_data_full(
        G_OBJECT(popover), "gc-context-menu-session", session,
        [](gpointer value) {
          delete static_cast<ContextMenuSession*>(value);
        });
    g_signal_connect(popover, "closed",
                     G_CALLBACK(on_context_popover_closed), session);

    auto* box = build_context_popover_box(request.items, session);
    gtk_container_add(GTK_CONTAINER(popover), box);

    GdkRectangle anchor{request.x, request.y, 1, 1};
    gtk_popover_set_pointing_to(GTK_POPOVER(popover), &anchor);
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
    active_context_popover = popover;
    gtk_widget_show_all(popover);
    gtk_popover_popup(GTK_POPOVER(popover));
    g_timeout_add(750, on_context_menu_stability_check,
                  g_object_ref(popover));

    if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
      std::cerr << "[GoreeCloud GTK] windowless-context-menu-presented items="
                << request.items.size()
                << " x=" << request.x << " y=" << request.y
                << " host=popover" << std::endl;
    }
  }

  void present_software_frame(const NativeSurfaceFrame& frame) override {
    if (!frame.bgra || frame.width <= 0 || frame.height <= 0 ||
        frame.stride < frame.width * 4) {
      return;
    }

    {
      std::scoped_lock lock(software_frame_mutex);
      software_frame.assign(
          frame.bgra,
          frame.bgra + static_cast<std::size_t>(frame.stride) *
                           static_cast<std::size_t>(frame.height));
      software_frame_width = frame.width;
      software_frame_height = frame.height;
      software_frame_stride = frame.stride;
    }

    if (!software_frame_diagnostic_emitted &&
        environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS") &&
        frame_has_visual_content(frame)) {
      software_frame_diagnostic_emitted = true;
      std::cerr << "[GoreeCloud GTK] windowless-frame-presented nonuniform=yes size="
                << frame.width << "x" << frame.height
                << " scale=" << frame.scale_factor << std::endl;
    }

    if (content_area) gtk_widget_queue_draw(content_area);
  }

  void present_software_popup_frame(const NativeSurfaceFrame& frame,
                                    NativePopupRect rect) override {
    if (!frame.bgra || frame.width <= 0 || frame.height <= 0 ||
        frame.stride < frame.width * 4 ||
        rect.width <= 0 || rect.height <= 0) {
      return;
    }

    {
      std::scoped_lock lock(software_frame_mutex);
      software_popup_frame.assign(
          frame.bgra,
          frame.bgra + static_cast<std::size_t>(frame.stride) *
                           static_cast<std::size_t>(frame.height));
      software_popup_width = frame.width;
      software_popup_height = frame.height;
      software_popup_stride = frame.stride;
      software_popup_rect = rect;
    }

    if (environment_flag_enabled("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS")) {
      std::cerr << "[GoreeCloud GTK] windowless-popup-frame-presented pixels="
                << frame.width << "x" << frame.height
                << " rect=" << rect.x << "," << rect.y << " "
                << rect.width << "x" << rect.height << std::endl;
    }

    if (content_area) gtk_widget_queue_draw(content_area);
  }

  void clear_software_popup_frame() override {
    {
      std::scoped_lock lock(software_frame_mutex);
      software_popup_frame.clear();
      software_popup_width = 0;
      software_popup_height = 0;
      software_popup_stride = 0;
      software_popup_rect = {};
    }
    if (content_area) gtk_widget_queue_draw(content_area);
  }

  static bool frame_has_visual_content(const NativeSurfaceFrame& frame) {
    if (!frame.bgra || frame.width <= 0 || frame.height <= 0 ||
        frame.stride < frame.width * 4) {
      return false;
    }

    const auto* first = frame.bgra;
    for (int y = 0; y < frame.height; ++y) {
      const auto* row =
          frame.bgra + static_cast<std::size_t>(y) * frame.stride;
      for (int x = 0; x < frame.width; ++x) {
        const auto* pixel = row + static_cast<std::size_t>(x) * 4;
        if (pixel[0] != first[0] || pixel[1] != first[1] ||
            pixel[2] != first[2]) {
          return true;
        }
      }
    }
    return false;
  }

  gboolean draw_software_frame(GtkWidget* widget, cairo_t* cr) {
    std::scoped_lock lock(software_frame_mutex);
    if (software_frame.empty() || software_frame_width <= 0 ||
        software_frame_height <= 0 || software_frame_stride <= 0) {
      return FALSE;
    }

    auto* image = cairo_image_surface_create_for_data(
        software_frame.data(), CAIRO_FORMAT_ARGB32, software_frame_width,
        software_frame_height, software_frame_stride);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
      cairo_surface_destroy(image);
      return FALSE;
    }

    GtkAllocation allocation{};
    gtk_widget_get_allocation(widget, &allocation);
    const double scale_x =
        static_cast<double>(allocation.width) /
        static_cast<double>(software_frame_width);
    const double scale_y =
        static_cast<double>(allocation.height) /
        static_cast<double>(software_frame_height);

    cairo_save(cr);
    cairo_scale(cr, scale_x, scale_y);
    cairo_set_source_surface(cr, image, 0.0, 0.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_restore(cr);
    cairo_surface_destroy(image);

    if (!software_popup_frame.empty() && software_popup_width > 0 &&
        software_popup_height > 0 && software_popup_stride > 0 &&
        software_popup_rect.width > 0 && software_popup_rect.height > 0) {
      auto* popup = cairo_image_surface_create_for_data(
          software_popup_frame.data(), CAIRO_FORMAT_ARGB32,
          software_popup_width, software_popup_height,
          software_popup_stride);
      if (cairo_surface_status(popup) == CAIRO_STATUS_SUCCESS) {
        const auto displayed =
            clamp_popup_rect(software_popup_rect, allocation.width,
                             allocation.height);
        if (displayed.width > 0 && displayed.height > 0) {
          cairo_save(cr);
          cairo_translate(cr, displayed.x, displayed.y);
          cairo_scale(
              cr,
              static_cast<double>(displayed.width) /
                  static_cast<double>(software_popup_width),
              static_cast<double>(displayed.height) /
                  static_cast<double>(software_popup_height));
          cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
          cairo_set_source_surface(cr, popup, 0.0, 0.0);
          cairo_paint(cr);
          cairo_restore(cr);
        }
      }
      cairo_surface_destroy(popup);
    }

    return TRUE;
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
      .gc-tab-list { min-height: 38px; }
      .gc-browser-tab {
        min-height: 36px;
        margin-right: 4px;
        padding: 2px 4px 2px 8px;
        border-radius: 13px;
        border: 1px solid transparent;
        background: transparent;
      }
      .gc-browser-tab-active {
        background-color: alpha(@theme_base_color, 0.90);
        border-color: alpha(@theme_fg_color, 0.12);
        box-shadow: 0 2px 8px alpha(#000000, 0.06);
      }
      .gc-tab-select {
        min-height: 34px;
        min-width: 118px;
        padding: 0 8px;
        border: none;
        background: transparent;
        box-shadow: none;
        font-weight: 600;
      }
      .gc-tab-select:hover { background-color: alpha(@theme_fg_color, 0.05); }
      .gc-tab-close, .gc-new-tab {
        min-width: 34px;
        min-height: 34px;
        padding: 0;
        border-radius: 10px;
        border: none;
        background: transparent;
        box-shadow: none;
      }
      .gc-tab-close:hover, .gc-new-tab:hover {
        background-color: alpha(@theme_fg_color, 0.07);
      }
      .gc-stage-badge {
        opacity: 0.74;
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
      popover.gc-context-popover > contents {
        border-radius: 14px;
        background-color: alpha(@theme_bg_color, 0.98);
        border: 1px solid alpha(@theme_fg_color, 0.12);
      }
      .gc-context-popover-box {
        padding: 6px;
      }
      .gc-context-row {
        min-height: 34px;
        padding: 4px 10px;
        border-radius: 9px;
      }
      .gc-context-row:hover {
        background-color: alpha(@theme_selected_bg_color, 0.16);
      }
      .gc-context-separator {
        margin: 3px 4px;
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
      .gc-internal-search {
        min-height: 52px;
        margin-top: 8px;
        padding: 0 16px;
        border-radius: 18px;
        background-color: alpha(@theme_base_color, 0.92);
        border: 1px solid alpha(@theme_fg_color, 0.12);
        box-shadow: 0 3px 12px alpha(#000000, 0.06);
      }
      .gc-quick-actions { margin-top: 4px; }
      .gc-quick-action {
        min-height: 42px;
        padding: 5px 12px;
        border-radius: 13px;
        background-color: alpha(@theme_fg_color, 0.04);
        border: 1px solid alpha(@theme_fg_color, 0.08);
        box-shadow: none;
      }
      .gc-quick-action:hover {
        background-color: alpha(@theme_fg_color, 0.07);
      }
      .gc-settings-grid {
        margin-top: 8px;
      }
      .gc-settings-section {
        min-width: 150px;
        min-height: 44px;
        padding: 10px 14px;
        border-radius: 14px;
        background-color: alpha(@theme_fg_color, 0.035);
        border: 1px solid alpha(@theme_fg_color, 0.08);
      }
      .gc-panel-eyebrow {
        opacity: 0.68;
        font-size: 0.82em;
        font-weight: 700;
      }
      .gc-panel-status {
        margin-top: 4px;
      }
      .gc-panel-header {
        margin-bottom: 2px;
      }
      .gc-panel-close {
        min-width: 36px;
        min-height: 36px;
        padding: 0;
        border-radius: 12px;
        border: 1px solid transparent;
        background: transparent;
        box-shadow: none;
      }
      .gc-panel-close:hover {
        background-color: alpha(@theme_fg_color, 0.07);
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
    if (!display) return false;

    install_css();
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 800);
    gtk_window_set_title(GTK_WINDOW(window), "GoreeCloud Browser");
    add_style_class(window, "gc-browser-window");
    if (private_window) add_style_class(window, "gc-private-window");
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);
    g_signal_connect(window, "key-press-event",
                     G_CALLBACK(on_window_key_press), this);

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
    im_context = gtk_im_multicontext_new();
    gtk_im_context_set_use_preedit(im_context, TRUE);
    g_signal_connect(im_context, "commit",
                     G_CALLBACK(on_im_commit), this);
    g_signal_connect(im_context, "preedit-changed",
                     G_CALLBACK(on_im_preedit_changed), this);
    g_signal_connect(im_context, "preedit-end",
                     G_CALLBACK(on_im_preedit_end), this);
    gtk_widget_set_hexpand(content_area, TRUE);
    gtk_widget_set_vexpand(content_area, TRUE);
    gtk_stack_add_named(GTK_STACK(content_stack), content_area, "web");
    g_signal_connect(content_area, "size-allocate",
                     G_CALLBACK(on_content_size_allocate), this);
    g_signal_connect(content_area, "draw",
                     G_CALLBACK(on_content_draw), this);
    gtk_widget_set_can_focus(content_area, TRUE);
    gtk_widget_add_events(
        content_area,
        GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK |
            GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
            GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK |
            GDK_KEY_RELEASE_MASK);
    g_signal_connect(content_area, "motion-notify-event",
                     G_CALLBACK(on_content_motion), this);
    g_signal_connect(content_area, "enter-notify-event",
                     G_CALLBACK(on_content_enter), this);
    g_signal_connect(content_area, "leave-notify-event",
                     G_CALLBACK(on_content_leave), this);
    g_signal_connect(content_area, "button-press-event",
                     G_CALLBACK(on_content_button), this);
    g_signal_connect(content_area, "button-release-event",
                     G_CALLBACK(on_content_button), this);
    g_signal_connect(content_area, "scroll-event",
                     G_CALLBACK(on_content_scroll), this);
    g_signal_connect(content_area, "key-press-event",
                     G_CALLBACK(on_content_key), this);
    g_signal_connect(content_area, "key-release-event",
                     G_CALLBACK(on_content_key), this);
    g_signal_connect(content_area, "focus-in-event",
                     G_CALLBACK(on_content_focus), this);
    g_signal_connect(content_area, "focus-out-event",
                     G_CALLBACK(on_content_focus), this);

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

    tab_list = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_style_class(tab_list, "gc-tab-list");
    gtk_box_pack_start(GTK_BOX(tab_strip), tab_list, FALSE, FALSE, 0);

    new_tab_button = gtk_button_new();
    set_button_icon(new_tab_button, "+", "list-add-symbolic");
    gtk_widget_set_size_request(new_tab_button, 34, 34);
    add_style_class(new_tab_button, "gc-new-tab");
    set_accessible_name(new_tab_button, "New Tab");
    g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab_clicked), this);
    gtk_box_pack_start(GTK_BOX(tab_strip), new_tab_button, FALSE, FALSE, 0);

    auto* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(tab_strip), spacer, TRUE, TRUE, 0);

    stage_badge = gtk_label_new(
        private_window ? "Private • Development" : "Development • Glaze 1.6");
    add_style_class(stage_badge, "gc-stage-badge");
    gtk_box_pack_end(GTK_BOX(tab_strip), stage_badge, FALSE, FALSE, 0);
  }

  void render_tabs(const BrowserChromeState& state) {
    if (!tab_list) return;

    active_tab_id.clear();
    for (const auto& tab : state.tabs) {
      if (tab.active) {
        active_tab_id = tab.id;
        break;
      }
    }

    std::string signature;
    for (const auto& tab : state.tabs) {
      signature += tab.id;
      signature.push_back('|');
      signature += tab.title;
      signature.push_back('|');
      signature += tab.active ? "1" : "0";
      signature += tab.loading ? "1" : "0";
      signature.push_back(';');
    }
    if (signature == tab_signature) return;
    tab_signature = std::move(signature);

    auto* children = gtk_container_get_children(GTK_CONTAINER(tab_list));
    for (auto* node = children; node; node = node->next) {
      gtk_widget_destroy(GTK_WIDGET(node->data));
    }
    g_list_free(children);

    for (const auto& tab : state.tabs) {
      auto* shell = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
      add_style_class(shell, "gc-browser-tab");
      if (tab.active) add_style_class(shell, "gc-browser-tab-active");

      auto* select = gtk_button_new_with_label(tab.title.c_str());
      add_style_class(select, "gc-tab-select");
      set_accessible_name(select, tab.title.c_str());
      g_object_set_data_full(G_OBJECT(select), "gc-tab-id",
                             g_strdup(tab.id.c_str()), g_free);
      g_signal_connect(select, "clicked", G_CALLBACK(on_tab_activate_clicked), this);
      if (auto* child = gtk_bin_get_child(GTK_BIN(select));
          child && GTK_IS_LABEL(child)) {
        gtk_label_set_ellipsize(GTK_LABEL(child), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(child), 22);
      }
      gtk_box_pack_start(GTK_BOX(shell), select, TRUE, TRUE, 0);

      auto* close = gtk_button_new();
      set_button_icon(close, "×", "window-close-symbolic");
      gtk_widget_set_size_request(close, 34, 34);
      add_style_class(close, "gc-tab-close");
      set_accessible_name(close, "Close Tab");
      g_object_set_data_full(G_OBJECT(close), "gc-tab-id",
                             g_strdup(tab.id.c_str()), g_free);
      g_signal_connect(close, "clicked", G_CALLBACK(on_tab_close_clicked), this);
      gtk_box_pack_start(GTK_BOX(shell), close, FALSE, FALSE, 0);

      gtk_box_pack_start(GTK_BOX(tab_list), shell, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(tab_list);
  }

  void build_toolbar() {
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    add_style_class(toolbar, "gc-toolbar");
    gtk_box_pack_start(GTK_BOX(chrome_shell), toolbar, FALSE, FALSE, 0);

    add_toolbar_button(ToolbarItem::back, "←", "go-previous-symbolic", "Back");
    add_toolbar_button(ToolbarItem::forward, "→", "go-next-symbolic", "Forward");
    add_toolbar_button(ToolbarItem::refresh, "↻", "view-refresh-symbolic",
                       "Refresh or Stop");
    add_toolbar_button(ToolbarItem::home, "⌂", "go-home-symbolic", "Home");
    build_unified_search();
    add_toolbar_button(ToolbarItem::advanced_download_manager, "↓",
                       "document-save-symbolic", "Advanced Download Manager");
    build_overflow_menu();
    add_toolbar_button(ToolbarItem::settings, "⚙",
                       "preferences-system-symbolic", "Settings");
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
    set_button_icon(overflow_menu, "⋯", "open-menu-symbolic");
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

  void add_toolbar_button(ToolbarItem item, const char* fallback,
                          const char* icon_name, const char* accessible) {
    auto* button = make_toolbar_button(fallback, icon_name, accessible);
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
    gtk_widget_set_valign(internal_card, GTK_ALIGN_START);
    gtk_widget_set_margin_top(internal_card, 34);
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

    internal_search_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(internal_search_entry),
                                   "Search or enter an address");
    add_style_class(internal_search_entry, "gc-internal-search");
    set_accessible_name(internal_search_entry, "Search or enter an address");
    g_signal_connect(internal_search_entry, "activate",
                     G_CALLBACK(on_search_activate), this);
    gtk_box_pack_start(GTK_BOX(internal_card), internal_search_entry,
                       FALSE, FALSE, 0);

    internal_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 7);
    add_style_class(internal_actions, "gc-quick-actions");

    auto* bookmarks = gtk_button_new_with_label("Bookmarks");
    add_style_class(bookmarks, "gc-quick-action");
    set_accessible_name(bookmarks, "Unified Bookmarks");
    search_control_bindings.emplace(bookmarks,
                                    UnifiedSearchBarControl::unified_bookmarks);
    g_signal_connect(bookmarks, "clicked",
                     G_CALLBACK(on_search_control_clicked), this);
    gtk_box_pack_start(GTK_BOX(internal_actions), bookmarks, FALSE, FALSE, 0);

    auto* downloads = gtk_button_new_with_label("Downloads");
    add_style_class(downloads, "gc-quick-action");
    set_accessible_name(downloads, "Advanced Download Manager");
    toolbar_bindings.emplace(downloads, ToolbarItem::advanced_download_manager);
    g_signal_connect(downloads, "clicked", G_CALLBACK(on_toolbar_clicked), this);
    gtk_box_pack_start(GTK_BOX(internal_actions), downloads, FALSE, FALSE, 0);

    auto* settings = gtk_button_new_with_label("Settings");
    add_style_class(settings, "gc-quick-action");
    set_accessible_name(settings, "Settings");
    toolbar_bindings.emplace(settings, ToolbarItem::settings);
    g_signal_connect(settings, "clicked", G_CALLBACK(on_toolbar_clicked), this);
    gtk_box_pack_start(GTK_BOX(internal_actions), settings, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(internal_card), internal_actions,
                       FALSE, FALSE, 0);

    settings_grid = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(settings_grid), GTK_SELECTION_NONE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(settings_grid), 8);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(settings_grid), 8);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(settings_grid), 1);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(settings_grid), 3);
    add_style_class(settings_grid, "gc-settings-grid");
    constexpr std::array<const char*, 9> kVisibleSettingsSections{
        "General", "Appearance", "Search",
        "Privacy & blocking", "Wardveil Security", "Downloads",
        "Network & DNS", "Accessibility", "Advanced"};
    for (const auto* section : kVisibleSettingsSections) {
      auto* label = gtk_label_new(section);
      gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
      add_style_class(label, "gc-settings-section");
      set_accessible_name(label, section);
      gtk_flow_box_insert(GTK_FLOW_BOX(settings_grid), label, -1);
    }
    gtk_box_pack_start(GTK_BOX(internal_card), settings_grid, FALSE, FALSE, 0);

    auto* status_row = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(status_row), GTK_SELECTION_NONE);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(status_row), 7);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(status_row), 7);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(status_row), 1);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(status_row), 1);
    add_style_class(status_row, "gc-status-row");
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
    gtk_widget_set_valign(panel_card, GTK_ALIGN_START);
    gtk_widget_set_margin_top(panel_card, 44);
    gtk_widget_set_size_request(panel_card, 420, -1);
    add_style_class(panel_card, "gc-panel-card");
    gtk_box_pack_start(GTK_BOX(panel_canvas), panel_card, TRUE, FALSE, 0);

    auto* panel_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    add_style_class(panel_header, "gc-panel-header");

    panel_eyebrow = gtk_label_new("GOREECLOUD BROWSER");
    gtk_label_set_xalign(GTK_LABEL(panel_eyebrow), 0.0F);
    add_style_class(panel_eyebrow, "gc-panel-eyebrow");
    gtk_box_pack_start(GTK_BOX(panel_header), panel_eyebrow, TRUE, TRUE, 0);

    panel_close = gtk_button_new();
    set_button_icon(panel_close, "×", "window-close-symbolic");
    add_style_class(panel_close, "gc-panel-close");
    set_accessible_name(panel_close, "Close panel");
    g_signal_connect(panel_close, "clicked",
                     G_CALLBACK(on_panel_close_clicked), this);
    gtk_box_pack_end(GTK_BOX(panel_header), panel_close, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel_card), panel_header, FALSE, FALSE, 0);

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

    panel_status = make_status_chip("Development surface");
    add_style_class(panel_status, "gc-panel-status");
    gtk_box_pack_start(GTK_BOX(panel_card), panel_status, FALSE, FALSE, 0);

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

    const bool launch_surface = url == kNewTabUrl || url == kHomeUrl;
    const bool settings_surface = url == kSettingsUrl;
    if (internal_search_entry) {
      gtk_widget_set_visible(internal_search_entry, launch_surface);
      if (launch_surface) gtk_entry_set_text(GTK_ENTRY(internal_search_entry), "");
    }
    if (internal_actions) gtk_widget_set_visible(internal_actions, launch_surface);
    if (settings_grid) gtk_widget_set_visible(settings_grid, settings_surface);
  }

  void show_renderer_compatibility_surface() {
    if (!content_stack || !internal_canvas) return;
    if (internal_eyebrow) {
      gtk_label_set_text(GTK_LABEL(internal_eyebrow), "LINUX RENDERER");
    }
    if (internal_title) {
      gtk_label_set_text(GTK_LABEL(internal_title),
                         "The web renderer is unavailable in this session.");
    }
    if (internal_subtitle) {
      gtk_label_set_text(
          GTK_LABEL(internal_subtitle),
          "GoreeCloud Browser could not attach either its native child surface "
          "or its software-rendered page surface. The Browser shell remains "
          "available so the failure stays visible and recoverable.");
    }
    if (internal_status) {
      gtk_label_set_text(GTK_LABEL(internal_status),
                         "Renderer attachment unavailable");
    }
    if (internal_search_entry) gtk_widget_set_visible(internal_search_entry, FALSE);
    if (internal_actions) gtk_widget_set_visible(internal_actions, FALSE);
    if (settings_grid) gtk_widget_set_visible(settings_grid, FALSE);
    gtk_stack_set_visible_child_name(GTK_STACK(content_stack), "internal");
  }

  void set_panel_surface_copy(std::string_view payload) {
    const auto presentation = browser_panel_presentation(payload);
    if (panel_eyebrow) {
      gtk_label_set_text(GTK_LABEL(panel_eyebrow), presentation.eyebrow.c_str());
    }
    if (panel_title) {
      gtk_label_set_text(GTK_LABEL(panel_title), presentation.title.c_str());
    }
    if (panel_label) {
      gtk_label_set_text(GTK_LABEL(panel_label), presentation.body.c_str());
    }
    if (panel_status) {
      gtk_label_set_text(GTK_LABEL(panel_status), presentation.status.c_str());
      gtk_widget_set_visible(panel_status, !presentation.status.empty());
    }
  }

  [[nodiscard]] bool panel_is_visible() const {
    if (!content_stack) return false;
    const auto* name =
        gtk_stack_get_visible_child_name(GTK_STACK(content_stack));
    return name && std::string_view{name} == "panel";
  }

  void close_panel() {
    if (!content_stack) return;
    const auto target = panel_return_child.empty() ? std::string{"internal"}
                                                   : panel_return_child;
    gtk_stack_set_visible_child_name(GTK_STACK(content_stack), target.c_str());
  }

  void show() {
    if (!created || !window) return;
    gtk_widget_show_all(window);
    gtk_widget_realize(content_area);
    update_metrics();
    if (attached_view) attach_engine_surface();
  }

  NativeEngineSurface current_surface() {
    NativeEngineSurface surface;
    if (!content_area || !gtk_widget_get_realized(content_area)) return surface;

    GtkAllocation allocation{};
    gtk_widget_get_allocation(content_area, &allocation);
    surface.x = 0;
    surface.y = 0;
    surface.width = allocation.width;
    surface.height = allocation.height;
    surface.scale_factor =
        static_cast<float>(gtk_widget_get_scale_factor(content_area));

    auto* gdk_window = gtk_widget_get_window(content_area);
    if (!gdk_window) return surface;
    auto* display = gdk_window_get_display(gdk_window);
    if (!display) return surface;

    const bool force_windowless =
        environment_flag_enabled("GOREECLOUD_BROWSER_FORCE_WINDOWLESS");
    if (GDK_IS_X11_DISPLAY(display) && !force_windowless) {
      surface.window_handle =
          static_cast<std::uintptr_t>(gdk_x11_window_get_xid(gdk_window));
      surface.display_handle = reinterpret_cast<std::uintptr_t>(
          gdk_x11_display_get_xdisplay(display));
    } else {
      surface.frame_sink = this;
      surface.cursor_sink = this;
      surface.context_menu_sink = this;
    }
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
    if (surface.window_handle != 0) reset_native_cursor();
    if ((surface.window_handle == 0 && surface.frame_sink == nullptr) ||
        surface.width <= 0 || surface.height <= 0) {
      engine_surface_attached = false;
      show_renderer_compatibility_surface();
      return;
    }
    engine_surface_attached = attachable->attach_native_surface(surface);
    software_surface_attached =
        engine_surface_attached && surface.frame_sink != nullptr;
    if (engine_surface_attached) {
      gtk_stack_set_visible_child_name(GTK_STACK(content_stack), "web");
    } else {
      show_renderer_compatibility_surface();
    }
  }

  void resize_attached_engine() {
    if (!engine_surface_attached || !attached_view) return;
    auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(attached_view);
    if (!attachable) return;
    const auto surface = current_surface();
    if ((surface.window_handle != 0 || surface.frame_sink != nullptr) &&
        surface.width > 0 && surface.height > 0) {
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
  TabActionHandler tab_action_handler;
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
  GtkWidget* tab_list{nullptr};
  GtkWidget* new_tab_button{nullptr};
  GtkWidget* stage_badge{nullptr};
  GtkWidget* toolbar{nullptr};
  GtkWidget* search_shell{nullptr};
  GtkWidget* search_entry{nullptr};
  GtkWidget* overflow_menu{nullptr};
  GtkWidget* overflow_popover{nullptr};
  GtkWidget* content_stack{nullptr};
  GtkWidget* content_area{nullptr};
  GtkIMContext* im_context{nullptr};
  GtkWidget* active_context_popover{nullptr};
  std::optional<NativeCursorType> active_cursor;
  GtkWidget* internal_canvas{nullptr};
  GtkWidget* internal_card{nullptr};
  GtkWidget* internal_eyebrow{nullptr};
  GtkWidget* internal_title{nullptr};
  GtkWidget* internal_subtitle{nullptr};
  GtkWidget* internal_search_entry{nullptr};
  GtkWidget* internal_actions{nullptr};
  GtkWidget* settings_grid{nullptr};
  GtkWidget* internal_status{nullptr};
  GtkWidget* panel_canvas{nullptr};
  GtkWidget* panel_card{nullptr};
  GtkWidget* panel_eyebrow{nullptr};
  GtkWidget* panel_title{nullptr};
  GtkWidget* panel_label{nullptr};
  GtkWidget* panel_status{nullptr};
  GtkWidget* panel_close{nullptr};
  GtkCssProvider* css{nullptr};

  EngineView* attached_view{nullptr};
  NativeWindowMetrics metrics{1280, 800, 1.0F};
  std::mutex software_frame_mutex;
  std::vector<std::uint8_t> software_frame;
  int software_frame_width{0};
  int software_frame_height{0};
  int software_frame_stride{0};
  std::vector<std::uint8_t> software_popup_frame;
  int software_popup_width{0};
  int software_popup_height{0};
  int software_popup_stride{0};
  NativePopupRect software_popup_rect{};
  bool software_frame_diagnostic_emitted{false};
  guint media_hover_timer_id{0};
  std::string tab_signature;
  std::string active_tab_id;
  std::string panel_return_child{"internal"};
  bool private_window{false};
  bool created{false};
  bool close_requested{false};
  bool engine_surface_attached{false};
  bool software_surface_attached{false};
  bool ime_preedit_active{false};
};

GtkLinuxGlazeWindowHost::GtkLinuxGlazeWindowHost()
    : impl_(std::make_unique<Impl>()) {}

GtkLinuxGlazeWindowHost::~GtkLinuxGlazeWindowHost() {
  if (!impl_) return;
  impl_->stop_media_hover_timer();
  detach_engine_view();
  if (impl_->im_context) {
    g_object_unref(impl_->im_context);
    impl_->im_context = nullptr;
  }
  if (impl_->css) {
    g_object_unref(impl_->css);
    impl_->css = nullptr;
  }
}

void GtkLinuxGlazeWindowHost::set_toolbar_handler(ToolbarHandler handler) {
  impl_->toolbar_handler = std::move(handler);
}
void GtkLinuxGlazeWindowHost::set_tab_action_handler(TabActionHandler handler) {
  impl_->tab_action_handler = std::move(handler);
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
  impl_->render_tabs(state);
}

void GtkLinuxGlazeWindowHost::attach_engine_view(EngineView& view) {
  if (impl_->attached_view == &view) {
    if (!impl_->engine_surface_attached) {
      impl_->attach_engine_surface();
    } else if (impl_->content_stack) {
      gtk_stack_set_visible_child_name(GTK_STACK(impl_->content_stack), "web");
    }
    return;
  }

  detach_engine_view();
  impl_->attached_view = &view;
  impl_->media_hover.invalidate();
  impl_->attach_engine_surface();
}

void GtkLinuxGlazeWindowHost::detach_engine_view() {
  impl_->media_hover.invalidate();
  impl_->current_media_target.reset();
  impl_->reset_text_input();
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
  impl_->software_surface_attached = false;
  impl_->clear_software_popup_frame();
  impl_->reset_native_cursor();
}

void GtkLinuxGlazeWindowHost::show_internal_surface(
    std::string_view internal_url) {
  impl_->media_hover.invalidate();
  impl_->clear_software_popup_frame();
  impl_->reset_native_cursor();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (!impl_->content_stack || !impl_->internal_canvas) return;
  impl_->set_internal_surface_copy(internal_url);
  gtk_stack_set_visible_child_name(GTK_STACK(impl_->content_stack), "internal");
}

void GtkLinuxGlazeWindowHost::show_panel(std::string_view panel_id) {
  impl_->media_hover.invalidate();
  impl_->reset_native_cursor();
  if (impl_->content_area) hide_gtk_media_hover_popover(impl_->content_area);
  if (!impl_->content_stack || !impl_->panel_label) return;

  if (const auto* visible =
          gtk_stack_get_visible_child_name(GTK_STACK(impl_->content_stack));
      visible && std::string_view{visible} != "panel") {
    impl_->panel_return_child = visible;
  }

  impl_->set_panel_surface_copy(panel_id);
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
