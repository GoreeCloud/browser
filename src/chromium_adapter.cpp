#include "goreecloud/browser/chromium_adapter.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "goreecloud/browser/chromium_runtime_delegate.hpp"
#include "goreecloud/browser/chromium_runtime_factory.hpp"
#include "goreecloud/browser/media_hit_test_provider.hpp"
#include "goreecloud/browser/media_preview_provider.hpp"
#include "goreecloud/browser/native_engine_surface.hpp"

namespace goreecloud::browser {
namespace {

EngineMediaElementKind to_engine_media_kind(MediaKind kind) {
  switch (kind) {
    case MediaKind::image: return EngineMediaElementKind::image;
    case MediaKind::video: return EngineMediaElementKind::video;
    case MediaKind::animated_image: return EngineMediaElementKind::animated_image;
    case MediaKind::thumbnail: return EngineMediaElementKind::thumbnail;
    case MediaKind::background_image: return EngineMediaElementKind::background_image;
    case MediaKind::embedded_media: return EngineMediaElementKind::embedded_media;
    case MediaKind::unknown: break;
  }
  return EngineMediaElementKind::none;
}

EngineMediaHitTest to_engine_hit_test(const RawMediaHitTest& raw) {
  EngineMediaHitTest hit;
  hit.kind = to_engine_media_kind(raw.kind);
  hit.page_url = raw.page_url;
  hit.media_url = raw.media_url;
  if (!raw.link_url.empty()) hit.link_url = raw.link_url;
  hit.mime_type = raw.mime_type;
  hit.alt_text = raw.alt_text;
  hit.intrinsic_width = raw.intrinsic_width;
  hit.intrinsic_height = raw.intrinsic_height;
  hit.displayed_width = raw.displayed_width;
  hit.displayed_height = raw.displayed_height;
  hit.duration_seconds = raw.duration_seconds;
  hit.animated = raw.animated;
  hit.secure_resource = raw.secure_resource;
  hit.downloadable = raw.downloadable;
  hit.copyable = raw.copyable;
  hit.capturable_frame = raw.frame_capture_supported;
  hit.protected_media = raw.protected_media;
  hit.drm_protected = raw.drm_protected;
  hit.same_origin_with_page = !raw.cross_origin;
  return hit;
}

class ChromiumEngineView final : public EngineView,
                                 public NativeSurfaceAttachable,
                                 public NativeSurfaceInputForwarder,
                                 public NativeSurfaceDragTargetForwarder,
                                 public NativeSurfaceTextInputForwarder,
                                 public AsyncMediaHitTestProvider,
                                 public AsyncMediaPreviewProvider {
 public:
  explicit ChromiumEngineView(std::unique_ptr<ChromiumRuntimeView> runtime_view)
      : runtime_view_(std::move(runtime_view)) {
    if (!runtime_view_) {
      throw std::invalid_argument("ChromiumEngineView requires a runtime view");
    }
  }

  void navigate(std::string_view url) override { runtime_view_->navigate(url); }
  void reload() override { runtime_view_->reload(true); }
  void stop() override { runtime_view_->stop(); }
  void go_back() override { runtime_view_->go_back(); }
  void go_forward() override { runtime_view_->go_forward(); }
  void set_zoom(double factor) override { runtime_view_->set_zoom(factor); }
  void find(std::string_view query, bool forward) override {
    runtime_view_->find(query, forward);
  }
  void stop_find() override { runtime_view_->stop_find(); }

  [[nodiscard]] NavigationState navigation_state() const override {
    return runtime_view_->navigation_state();
  }

  [[nodiscard]] RendererHealth renderer_health() const override {
    return runtime_view_->renderer_health();
  }

  bool attach_native_surface(const NativeEngineSurface& surface) override {
    attached_ = runtime_view_->attach_surface(surface);
    return attached_;
  }

  void resize_native_surface(const NativeEngineSurface& surface) override {
    if (!attached_) return;
    runtime_view_->resize_surface(surface);
  }

  void detach_native_surface() override {
    runtime_view_->detach_surface();
    attached_ = false;
  }

  [[nodiscard]] bool native_surface_attached() const noexcept override {
    return attached_;
  }

  bool send_pointer_move(const NativePointerEvent& event, bool leave) override {
    return attached_ && runtime_view_->send_pointer_move(event, leave);
  }

  bool send_pointer_button(const NativePointerEvent& event,
                           NativePointerButton button,
                           bool pressed,
                           int click_count) override {
    return attached_ &&
           runtime_view_->send_pointer_button(event, button, pressed, click_count);
  }

  bool send_pointer_wheel(const NativePointerEvent& event,
                          int delta_x,
                          int delta_y) override {
    return attached_ &&
           runtime_view_->send_pointer_wheel(event, delta_x, delta_y);
  }

  bool send_key_event(const NativeKeyEvent& event) override {
    return attached_ && runtime_view_->send_key_event(event);
  }

  bool drag_target_enter(NativeDragData data,
                         const NativePointerEvent& event,
                         std::uint32_t allowed_operations) override {
    return attached_ &&
           runtime_view_->drag_target_enter(
               std::move(data), event, allowed_operations);
  }

  bool drag_target_over(const NativePointerEvent& event,
                        std::uint32_t allowed_operations) override {
    return attached_ &&
           runtime_view_->drag_target_over(event, allowed_operations);
  }

  void drag_target_leave() override {
    if (attached_) runtime_view_->drag_target_leave();
  }

  bool drag_target_drop(const NativePointerEvent& event) override {
    return attached_ && runtime_view_->drag_target_drop(event);
  }

  bool set_text_composition(std::u16string text,
                            int selection_start,
                            int selection_end) override {
    return attached_ &&
           runtime_view_->set_text_composition(
               text, selection_start, selection_end);
  }

  bool commit_text(std::u16string text) override {
    return attached_ && runtime_view_->commit_text(text);
  }

  void cancel_text_composition() override {
    if (attached_) runtime_view_->cancel_text_composition();
  }

  void set_surface_focus(bool focused) override {
    if (attached_) runtime_view_->set_surface_focus(focused);
  }

  bool request_media_hit_test(MediaHitTestPoint point,
                              std::uint64_t sequence,
                              ResultCallback callback) override {
    return runtime_view_->request_media_probe(
        point.viewport_x,
        point.viewport_y,
        sequence,
        [callback = std::move(callback)](
            std::uint64_t response_sequence,
            std::optional<RawMediaHitTest> raw) mutable {
          if (!callback) return;
          if (!raw) {
            callback(response_sequence, std::nullopt);
            return;
          }
          callback(response_sequence, to_engine_hit_test(*raw));
        });
  }

  bool request_media_preview(const MediaPreviewRequest& request,
                             PreviewCallback callback) override {
    if (request.target.protected_media) {
      if (callback) callback(std::nullopt, "Protected media preview is restricted by the engine.");
      return true;
    }
    return runtime_view_->request_media_preview(request, std::move(callback));
  }

 private:
  std::unique_ptr<ChromiumRuntimeView> runtime_view_;
  bool attached_{false};
};

class ChromiumEngineContext final : public EngineContext {
 public:
  explicit ChromiumEngineContext(std::unique_ptr<ChromiumRuntimeContext> runtime_context)
      : runtime_context_(std::move(runtime_context)) {
    if (!runtime_context_) {
      throw std::invalid_argument("ChromiumEngineContext requires a runtime context");
    }
  }

  [[nodiscard]] std::unique_ptr<EngineView> create_view(
      const EngineViewOptions& options) override {
    auto runtime_view = runtime_context_->create_view(options);
    if (!runtime_view) return nullptr;
    return std::make_unique<ChromiumEngineView>(std::move(runtime_view));
  }

  bool clear_origin_data(std::string_view origin,
                         EngineDataClasses classes) override {
    return runtime_context_->clear_origin_data(origin, classes);
  }

  bool clear_all_data(EngineDataClasses classes) override {
    return runtime_context_->clear_all_data(classes);
  }

  bool clear_authentication_state(std::optional<std::string_view> origin) override {
    return runtime_context_->clear_authentication_state(origin);
  }

  bool clear_permission_state(std::optional<std::string_view> origin) override {
    return runtime_context_->clear_permission_state(origin);
  }

 private:
  std::unique_ptr<ChromiumRuntimeContext> runtime_context_;
};

}  // namespace

class ChromiumEngineAdapter::Impl {
 public:
  explicit Impl(ChromiumAdapterOptions options)
      : options_(std::move(options)),
        runtime_(create_chromium_runtime_delegate(options_)) {
    if (!runtime_) {
      throw std::runtime_error("Failed to create Chromium runtime delegate");
    }
  }

  void initialize() {
    if (initialized_) return;
    if (options_.runtime_root.empty()) {
      throw std::runtime_error("Chromium runtime root is not configured");
    }
    if (!options_.enable_sandbox) {
      throw std::runtime_error(
          "GoreeCloud Browser refuses to initialize Chromium with the sandbox disabled");
    }
    if (!options_.enable_site_isolation) {
      throw std::runtime_error(
          "GoreeCloud Browser refuses to initialize Chromium with site isolation disabled");
    }
    runtime_->initialize();
    initialized_ = true;
  }

  void shutdown() noexcept {
    if (!initialized_) return;
    runtime_->shutdown();
    initialized_ = false;
  }

  void pump_events() {
    if (initialized_) runtime_->do_message_loop_work();
  }

  [[nodiscard]] bool initialized() const noexcept { return initialized_; }
  [[nodiscard]] ChromiumRuntimeDelegate& runtime() noexcept { return *runtime_; }
  [[nodiscard]] const ChromiumRuntimeDelegate& runtime() const noexcept {
    return *runtime_;
  }

 private:
  ChromiumAdapterOptions options_;
  std::unique_ptr<ChromiumRuntimeDelegate> runtime_;
  bool initialized_{false};
};

ChromiumEngineAdapter::ChromiumEngineAdapter(ChromiumAdapterOptions options)
    : impl_(std::make_unique<Impl>(std::move(options))) {}

ChromiumEngineAdapter::~ChromiumEngineAdapter() = default;

void ChromiumEngineAdapter::initialize() { impl_->initialize(); }
void ChromiumEngineAdapter::shutdown() { impl_->shutdown(); }
void ChromiumEngineAdapter::pump_events() { impl_->pump_events(); }

std::string_view ChromiumEngineAdapter::name() const noexcept {
  return "GoreeCloud Browser Engine Layer (Chromium)";
}

std::string_view ChromiumEngineAdapter::version() const noexcept {
  return impl_->runtime().runtime_version();
}

EngineCapabilities ChromiumEngineAdapter::capabilities() const noexcept {
  return capability(EngineCapability::WebView) |
         capability(EngineCapability::PrivateContexts) |
         capability(EngineCapability::SiteDataControl) |
         capability(EngineCapability::CookieInterception) |
         capability(EngineCapability::PermissionInterception) |
         capability(EngineCapability::DownloadInterception) |
         capability(EngineCapability::CertificateInspection) |
         capability(EngineCapability::RendererHealth) |
         capability(EngineCapability::HardwareAcceleration) |
         capability(EngineCapability::WebRTC) |
         capability(EngineCapability::Printing) |
         capability(EngineCapability::Accessibility) |
         capability(EngineCapability::PrivateContextCleanup) |
         capability(EngineCapability::OriginScopedCleanup) |
         capability(EngineCapability::AuthenticationStateCleanup) |
         capability(EngineCapability::PermissionStateCleanup);
}

std::unique_ptr<EngineContext> ChromiumEngineAdapter::create_context(
    const EngineContextOptions& options) {
  if (!impl_->initialized()) {
    throw std::logic_error(
        "Chromium adapter must be initialized before creating contexts");
  }
  if (options.private_context && options.persistent_storage) {
    throw std::invalid_argument(
        "Private Chromium contexts must use non-persistent storage");
  }
  if (!options.private_context && options.persistent_storage &&
      options.storage_path.empty()) {
    throw std::invalid_argument(
        "Persistent Chromium contexts require a storage path");
  }

  auto runtime_context = impl_->runtime().create_context(options);
  if (!runtime_context) return nullptr;
  return std::make_unique<ChromiumEngineContext>(std::move(runtime_context));
}

}  // namespace goreecloud::browser
