#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace goreecloud::browser {

struct NativeSurfaceFrame {
  const std::uint8_t* bgra{nullptr};
  int width{0};
  int height{0};
  int stride{0};
  float scale_factor{1.0F};
};

// Receives engine-owned software frames synchronously. Implementations must copy
// any pixels they need to retain after present_software_frame returns.
class NativeSurfaceFrameSink {
 public:
  virtual ~NativeSurfaceFrameSink() = default;
  virtual void present_software_frame(const NativeSurfaceFrame& frame) = 0;
};

enum class NativePointerButton {
  left,
  middle,
  right,
};

enum NativeInputModifier : std::uint32_t {
  native_modifier_none = 0,
  native_modifier_shift = 1U << 0,
  native_modifier_control = 1U << 1,
  native_modifier_alt = 1U << 2,
  native_modifier_left_button = 1U << 3,
  native_modifier_middle_button = 1U << 4,
  native_modifier_right_button = 1U << 5,
  native_modifier_caps_lock = 1U << 6,
};

struct NativePointerEvent {
  int x{0};
  int y{0};
  std::uint32_t modifiers{native_modifier_none};
};

enum class NativeKeyEventType {
  raw_key_down,
  key_up,
  character,
};

struct NativeKeyEvent {
  NativeKeyEventType type{NativeKeyEventType::raw_key_down};
  int virtual_key_code{0};
  int native_key_code{0};
  std::uint32_t modifiers{native_modifier_none};
  std::uint32_t character{0};
  std::uint32_t unmodified_character{0};
};

// Optional input bridge used by software/windowless engine surfaces. Native
// child-window surfaces continue receiving platform input directly.
class NativeSurfaceInputForwarder {
 public:
  virtual ~NativeSurfaceInputForwarder() = default;
  virtual bool send_pointer_move(const NativePointerEvent& event,
                                 bool leave) = 0;
  virtual bool send_pointer_button(const NativePointerEvent& event,
                                   NativePointerButton button,
                                   bool pressed,
                                   int click_count) = 0;
  virtual bool send_pointer_wheel(const NativePointerEvent& event,
                                  int delta_x,
                                  int delta_y) = 0;
  virtual bool send_key_event(const NativeKeyEvent& event) = 0;
  virtual void set_surface_focus(bool focused) = 0;
};

enum class NativeCursorType {
  pointer,
  crosshair,
  hand,
  text,
  wait,
  help,
  move,
  east_west_resize,
  north_south_resize,
  northeast_southwest_resize,
  northwest_southeast_resize,
  column_resize,
  row_resize,
  vertical_text,
  cell,
  context_menu,
  alias,
  progress,
  no_drop,
  copy,
  none,
  not_allowed,
  zoom_in,
  zoom_out,
  grab,
  grabbing,
};

// Optional cursor bridge for software/windowless engine surfaces. Native
// child-window surfaces retain platform/engine cursor handling.
class NativeSurfaceCursorSink {
 public:
  virtual ~NativeSurfaceCursorSink() = default;
  virtual void apply_native_cursor(NativeCursorType cursor) = 0;
};

enum class NativeContextMenuItemType {
  command,
  check,
  radio,
  separator,
  submenu,
};

struct NativeContextMenuItem {
  NativeContextMenuItemType type{NativeContextMenuItemType::command};
  int command_id{0};
  std::string label;
  bool enabled{true};
  bool checked{false};
  std::vector<NativeContextMenuItem> children;
};

struct NativeContextMenuRequest {
  int x{0};
  int y{0};
  std::vector<NativeContextMenuItem> items;
};

using NativeContextMenuSelectionCallback =
    std::function<void(std::optional<int> command_id)>;

// Optional Browser-host presentation bridge for software/windowless engine
// surfaces. Native child-window surfaces keep their engine/native menu path.
class NativeSurfaceContextMenuSink {
 public:
  virtual ~NativeSurfaceContextMenuSink() = default;
  virtual void show_native_context_menu(
      NativeContextMenuRequest request,
      NativeContextMenuSelectionCallback callback) = 0;
};

// Opaque platform surface description used to attach an engine-rendered view
// to GoreeCloud-owned native chrome without exposing Chromium types upstream.
//
// window_handle/display_handle describe the native child-window path. frame_sink
// describes the windowless/software path used when the host cannot provide a
// child window (for example a GTK Wayland backend).
struct NativeEngineSurface {
  std::uintptr_t window_handle{0};
  std::uintptr_t display_handle{0};
  int x{0};
  int y{0};
  int width{0};
  int height{0};
  float scale_factor{1.0F};
  NativeSurfaceFrameSink* frame_sink{nullptr};
  NativeSurfaceCursorSink* cursor_sink{nullptr};
  NativeSurfaceContextMenuSink* context_menu_sink{nullptr};
};

class NativeSurfaceAttachable {
 public:
  virtual ~NativeSurfaceAttachable() = default;
  virtual bool attach_native_surface(const NativeEngineSurface& surface) = 0;
  virtual void resize_native_surface(const NativeEngineSurface& surface) = 0;
  virtual void detach_native_surface() = 0;
  [[nodiscard]] virtual bool native_surface_attached() const noexcept = 0;
};

}  // namespace goreecloud::browser
