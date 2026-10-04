#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "goreecloud/browser/engine.hpp"
#include "goreecloud/browser/media_preview_provider.hpp"
#include "goreecloud/browser/media_target_detector.hpp"
#include "goreecloud/browser/native_engine_surface.hpp"

namespace goreecloud::browser {

class ChromiumRuntimeView {
 public:
  using MediaProbeCallback =
      std::function<void(std::uint64_t sequence,
                         std::optional<RawMediaHitTest> result)>;
  using MediaPreviewCallback = AsyncMediaPreviewProvider::PreviewCallback;

  virtual ~ChromiumRuntimeView() = default;
  virtual void navigate(std::string_view url) = 0;
  virtual void reload(bool bypass_cache) = 0;
  virtual void stop() = 0;
  virtual void go_back() = 0;
  virtual void go_forward() = 0;
  virtual void set_zoom(double factor) = 0;
  virtual void find(std::string_view query, bool forward) = 0;
  virtual void stop_find() = 0;
  virtual bool attach_surface(const NativeEngineSurface& surface) = 0;
  virtual void detach_surface() = 0;
  virtual void resize_surface(const NativeEngineSurface& surface) = 0;

  virtual bool send_pointer_move(const NativePointerEvent& event, bool leave) {
    (void)event;
    (void)leave;
    return false;
  }
  virtual bool send_pointer_button(const NativePointerEvent& event,
                                   NativePointerButton button,
                                   bool pressed,
                                   int click_count) {
    (void)event;
    (void)button;
    (void)pressed;
    (void)click_count;
    return false;
  }
  virtual bool send_pointer_wheel(const NativePointerEvent& event,
                                  int delta_x,
                                  int delta_y) {
    (void)event;
    (void)delta_x;
    (void)delta_y;
    return false;
  }
  virtual bool send_key_event(const NativeKeyEvent& event) {
    (void)event;
    return false;
  }
  virtual bool set_text_composition(const std::u16string& text,
                                    int selection_start,
                                    int selection_end) {
    (void)text;
    (void)selection_start;
    (void)selection_end;
    return false;
  }
  virtual bool commit_text(const std::u16string& text) {
    (void)text;
    return false;
  }
  virtual void cancel_text_composition() {}
  virtual void set_surface_focus(bool focused) { (void)focused; }

  // Optional asynchronous media probe. Runtime implementations that do not
  // support point media hit testing return false without invoking callback.
  virtual bool request_media_probe(int viewport_x,
                                   int viewport_y,
                                   std::uint64_t sequence,
                                   MediaProbeCallback callback) {
    (void)viewport_x;
    (void)viewport_y;
    (void)sequence;
    (void)callback;
    return false;
  }

  // Optional authenticated preview path. Implementations must derive preview
  // bytes from the active engine context and must not perform a second host-side
  // fetch that could bypass cookies, authorization state, DRM, CSP, CORS, or
  // other engine-enforced restrictions.
  virtual bool request_media_preview(const MediaPreviewRequest& request,
                                     MediaPreviewCallback callback) {
    (void)request;
    (void)callback;
    return false;
  }

  [[nodiscard]] virtual NavigationState navigation_state() const = 0;
  [[nodiscard]] virtual RendererHealth renderer_health() const = 0;
};

class ChromiumRuntimeContext {
 public:
  virtual ~ChromiumRuntimeContext() = default;
  [[nodiscard]] virtual std::unique_ptr<ChromiumRuntimeView> create_view(
      const EngineViewOptions& options) = 0;
  virtual bool clear_origin_data(std::string_view origin, EngineDataClasses classes) = 0;
  virtual bool clear_all_data(EngineDataClasses classes) = 0;
  virtual bool clear_authentication_state(std::optional<std::string_view> origin) = 0;
  virtual bool clear_permission_state(std::optional<std::string_view> origin) = 0;
};

class ChromiumRuntimeDelegate {
 public:
  virtual ~ChromiumRuntimeDelegate() = default;
  virtual void initialize() = 0;
  virtual void shutdown() = 0;
  [[nodiscard]] virtual std::unique_ptr<ChromiumRuntimeContext> create_context(
      const EngineContextOptions& options) = 0;
  virtual void do_message_loop_work() = 0;
  [[nodiscard]] virtual std::string_view runtime_version() const noexcept = 0;
};

}  // namespace goreecloud::browser
