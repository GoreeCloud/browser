#include "goreecloud/browser/cef_runtime.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <limits>
#include <optional>
#include <thread>
#include <stdexcept>
#include <utility>

#if GOREECLOUD_ENABLE_CEF
#include "goreecloud/browser/cef_browser_app.hpp"
#include "goreecloud/browser/cef_client.hpp"
#include "goreecloud/browser/cef_media_probe_app.hpp"
#include "include/cef_app.h"
#include "include/base/cef_callback.h"
#include "include/cef_browser.h"
#include "include/cef_drag_data.h"
#include "include/cef_request_context.h"
#include "include/wrapper/cef_closure_task.h"
#endif

namespace goreecloud::browser {

namespace {

#if GOREECLOUD_ENABLE_CEF

bool cef_runtime_diagnostics_enabled() {
  const char* value = std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
  return value && *value && std::string_view{value} != "0";
}

int cef_keyboard_modifiers(std::uint32_t modifiers) {
  int flags = EVENTFLAG_NONE;
  if (modifiers & native_modifier_shift) flags |= EVENTFLAG_SHIFT_DOWN;
  if (modifiers & native_modifier_control) flags |= EVENTFLAG_CONTROL_DOWN;
  if (modifiers & native_modifier_alt) flags |= EVENTFLAG_ALT_DOWN;
  if (modifiers & native_modifier_caps_lock) flags |= EVENTFLAG_CAPS_LOCK_ON;
  return flags;
}

int cef_mouse_modifiers(std::uint32_t modifiers) {
  int flags = cef_keyboard_modifiers(modifiers);
  if (modifiers & native_modifier_left_button) {
    flags |= EVENTFLAG_LEFT_MOUSE_BUTTON;
  }
  if (modifiers & native_modifier_middle_button) {
    flags |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
  }
  if (modifiers & native_modifier_right_button) {
    flags |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
  }
  return flags;
}

CefBrowserHost::MouseButtonType cef_mouse_button(NativePointerButton button) {
  switch (button) {
    case NativePointerButton::left:
      return MBT_LEFT;
    case NativePointerButton::middle:
      return MBT_MIDDLE;
    case NativePointerButton::right:
      return MBT_RIGHT;
  }
  return MBT_LEFT;
}

CefMouseEvent cef_mouse_event(const NativePointerEvent& event) {
  CefMouseEvent mouse_event;
  mouse_event.x = event.x;
  mouse_event.y = event.y;
  mouse_event.modifiers = cef_mouse_modifiers(event.modifiers);
  return mouse_event;
}

CefKeyEvent cef_key_event(const NativeKeyEvent& event) {
  CefKeyEvent key_event;
  switch (event.type) {
    case NativeKeyEventType::raw_key_down:
      key_event.type = KEYEVENT_RAWKEYDOWN;
      break;
    case NativeKeyEventType::key_up:
      key_event.type = KEYEVENT_KEYUP;
      break;
    case NativeKeyEventType::character:
      key_event.type = KEYEVENT_CHAR;
      break;
  }
  key_event.modifiers = cef_keyboard_modifiers(event.modifiers);
  key_event.windows_key_code = event.virtual_key_code;
  key_event.native_key_code = event.native_key_code;
  key_event.is_system_key = false;
  if (event.character <= 0xFFFFU) {
    key_event.character = static_cast<char16_t>(event.character);
  }
  if (event.unmodified_character <= 0xFFFFU) {
    key_event.unmodified_character =
        static_cast<char16_t>(event.unmodified_character);
  }
  return key_event;
}

const char* native_key_event_name(NativeKeyEventType type) {
  switch (type) {
    case NativeKeyEventType::raw_key_down:
      return "raw-key-down";
    case NativeKeyEventType::key_up:
      return "key-up";
    case NativeKeyEventType::character:
      return "character";
  }
  return "unknown";
}

class CefRuntimeView final : public ChromiumRuntimeView {
 public:
  CefRuntimeView(CefRefPtr<CefRequestContext> request_context,
                 EngineViewOptions options)
      : request_context_(std::move(request_context)), options_(std::move(options)) {
    state_.url = options_.initial_url;
  }

  void navigate(std::string_view url) override {
    {
      std::scoped_lock lock(state_mutex_);
      state_.url = std::string{url};
      state_.title = state_.url;
      state_.loading = true;
      state_.progress = 0.0;
    }
    if (client_ && client_->browser()) {
      client_->browser()->GetMainFrame()->LoadURL(std::string{url});
    }
  }

  void reload(bool bypass_cache) override {
    if (!client_ || !client_->browser()) return;
    if (bypass_cache) client_->browser()->ReloadIgnoreCache();
    else client_->browser()->Reload();
  }

  void stop() override {
    if (client_ && client_->browser()) client_->browser()->StopLoad();
  }

  void go_back() override {
    if (client_ && client_->browser() && client_->browser()->CanGoBack()) client_->browser()->GoBack();
  }

  void go_forward() override {
    if (client_ && client_->browser() && client_->browser()->CanGoForward()) client_->browser()->GoForward();
  }

  void set_zoom(double factor) override {
    if (client_ && client_->browser()) client_->browser()->GetHost()->SetZoomLevel(factor);
  }

  void find(std::string_view query, bool forward) override {
    if (!client_ || !client_->browser()) return;
    client_->browser()->GetHost()->Find(std::string{query}, forward, false, false);
  }

  void stop_find() override {
    if (client_ && client_->browser()) client_->browser()->GetHost()->StopFinding(true);
  }

  bool send_pointer_move(const NativePointerEvent& event, bool leave) override {
    if (!windowless_browser()) return false;
    client_->browser()->GetHost()->SendMouseMoveEvent(cef_mouse_event(event), leave);
    return true;
  }

  bool send_pointer_button(const NativePointerEvent& event,
                           NativePointerButton button,
                           bool pressed,
                           int click_count) override {
    if (!windowless_browser()) return false;
    client_->browser()->GetHost()->SendMouseClickEvent(
        cef_mouse_event(event), cef_mouse_button(button), !pressed,
        click_count > 0 ? click_count : 1);
    if (pressed) client_->browser()->GetHost()->SetFocus(true);
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-pointer-button button="
                << static_cast<int>(button)
                << " pressed=" << (pressed ? "yes" : "no")
                << " x=" << event.x << " y=" << event.y << std::endl;
    }
    return true;
  }

  bool send_pointer_wheel(const NativePointerEvent& event,
                          int delta_x,
                          int delta_y) override {
    if (!windowless_browser()) return false;
    client_->browser()->GetHost()->SendMouseWheelEvent(
        cef_mouse_event(event), delta_x, delta_y);
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-pointer-wheel dx="
                << delta_x << " dy=" << delta_y << std::endl;
    }
    return true;
  }

  bool send_key_event(const NativeKeyEvent& event) override {
    if (!windowless_browser()) return false;
    if (event.virtual_key_code <= 0 && event.type != NativeKeyEventType::character) {
      return false;
    }
    if (event.character > 0xFFFFU || event.unmodified_character > 0xFFFFU) {
      return false;
    }

    client_->browser()->GetHost()->SendKeyEvent(cef_key_event(event));
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-key-event type="
                << native_key_event_name(event.type)
                << " vkey=" << event.virtual_key_code
                << " native=" << event.native_key_code
                << " character=" << event.character << std::endl;
    }
    return true;
  }

  bool drop_data(NativeDropData data,
                 const NativePointerEvent& event,
                 NativeDropOperation operation) override {
    if (!windowless_browser()) return false;

    constexpr std::size_t kMaxDropTextBytes = 1024U * 1024U;
    constexpr std::size_t kMaxDropUrlBytes = 8192U;
    constexpr std::size_t kMaxDropTitleBytes = 4096U;

    const bool has_text = !data.text.empty();
    const bool has_link = !data.link_url.empty();
    if (has_text == has_link) return false;
    if (has_text && data.text.size() > kMaxDropTextBytes) return false;
    if (has_link &&
        (data.link_url.size() > kMaxDropUrlBytes ||
         data.link_title.size() > kMaxDropTitleBytes)) {
      return false;
    }
    if (has_text && operation != NativeDropOperation::copy) return false;
    if (has_link && operation != NativeDropOperation::link) return false;

    auto drag_data = CefDragData::Create();
    if (!drag_data) return false;

    CefBrowserHost::DragOperationsMask allowed_ops = DRAG_OPERATION_NONE;
    if (has_text) {
      drag_data->SetFragmentText(data.text);
      allowed_ops = DRAG_OPERATION_COPY;
    } else {
      drag_data->SetLinkURL(data.link_url);
      if (!data.link_title.empty()) drag_data->SetLinkTitle(data.link_title);
      allowed_ops = DRAG_OPERATION_LINK;
    }

    const auto mouse_event = cef_mouse_event(event);
    const bool started =
        client_->begin_windowless_drop(drag_data, mouse_event, allowed_ops);
    if (!started) return false;

    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-drop kind="
                << (has_link ? "link" : "text")
                << " x=" << event.x << " y=" << event.y
                << " bytes=" << (has_link ? data.link_url.size() : data.text.size())
                << " completion=awaiting-renderer" << std::endl;
    }
    return true;
  }

  bool set_text_composition(const std::u16string& text,
                            int selection_start,
                            int selection_end) override {
    if (!windowless_browser() ||
        text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
      return false;
    }

    const int text_length = static_cast<int>(text.size());
    const int start =
        selection_start < 0 ? 0 :
        (selection_start > text_length ? text_length : selection_start);
    const int end =
        selection_end < start ? start :
        (selection_end > text_length ? text_length : selection_end);

    client_->browser()->GetHost()->ImeSetComposition(
        CefString(text),
        {},
        CefRange::InvalidRange(),
        CefRange(start, end));
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-ime-composition length="
                << text.size() << " selection=" << start << ":" << end
                << std::endl;
    }
    return true;
  }

  bool commit_text(const std::u16string& text) override {
    if (!windowless_browser()) return false;
    client_->browser()->GetHost()->ImeCommitText(
        CefString(text), CefRange::InvalidRange(), 0);
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-ime-commit length="
                << text.size() << std::endl;
    }
    return true;
  }

  void cancel_text_composition() override {
    if (!windowless_browser()) return;
    client_->browser()->GetHost()->ImeCancelComposition();
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-ime-cancel" << std::endl;
    }
  }

  void set_surface_focus(bool focused) override {
    if (windowless_browser()) {
      client_->browser()->GetHost()->SetFocus(focused);
    }
  }

  bool request_media_probe(int viewport_x,
                           int viewport_y,
                           std::uint64_t sequence,
                           MediaProbeCallback callback) override {
    if (!client_ || !client_->browser() || !callback) return false;
    return client_->request_media_probe(viewport_x,
                                        viewport_y,
                                        sequence,
                                        std::move(callback));
  }

  bool request_media_preview(const MediaPreviewRequest& request,
                             MediaPreviewCallback callback) override {
    if (!client_ || !client_->browser() || !callback) return false;
    return client_->request_media_preview(request, std::move(callback));
  }

  bool attach_surface(const NativeEngineSurface& surface) override {
    if (surface.width <= 0 || surface.height <= 0) return false;

    const bool windowless =
        surface.window_handle == 0 && surface.frame_sink != nullptr;
    if (surface.window_handle == 0 && !windowless) return false;

    surface_ = surface;
    if (!client_) {
      client_ = new GoreeCloudCefClient(
          [this](const NavigationState& state) {
            std::scoped_lock lock(state_mutex_);
            state_ = state;
          },
          [this]() { closed_ = true; });
    }

    if (windowless) {
      client_->configure_windowless_surface(
          surface.frame_sink, surface.cursor_sink, surface.context_menu_sink,
          surface.text_input_geometry_sink, surface.width, surface.height,
          surface.scale_factor);
    } else {
      client_->clear_windowless_surface();
    }

    if (client_->browser()) {
      attached_ = true;
      client_->browser()->GetHost()->WasResized();
      return true;
    }

    CefWindowInfo window_info;
    if (windowless) {
      window_info.SetAsWindowless(static_cast<CefWindowHandle>(0));
      // Keep the current embedded Alloy runtime contract while using CEF's
      // software/off-screen rendering path for non-child-window hosts.
      window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    } else {
      window_info.SetAsChild(static_cast<CefWindowHandle>(surface.window_handle),
                             CefRect(surface.x, surface.y, surface.width,
                                     surface.height));
      // CEF 128+ uses the Chrome bootstrap. GoreeCloud embeds the browser into
      // its own GTK/X11 parent, so the child must explicitly use Alloy runtime
      // style while retaining the current Chrome bootstrap.
      window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
    }

    std::string initial_url;
    {
      std::scoped_lock lock(state_mutex_);
      initial_url = state_.url.empty() ? options_.initial_url : state_.url;
    }

    CefBrowserSettings browser_settings;
    if (windowless) browser_settings.windowless_frame_rate = 60;

    const bool created = CefBrowserHost::CreateBrowser(
        window_info,
        client_,
        initial_url,
        browser_settings,
        nullptr,
        request_context_);
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] CreateBrowser accepted="
                << (created ? "yes" : "no")
                << " mode=" << (windowless ? "windowless" : "child")
                << " url=" << initial_url
                << " parent=" << surface.window_handle
                << " size=" << surface.width << "x" << surface.height
                << std::endl;
    }
    attached_ = created;
    return created;
  }

  void detach_surface() override {
    if (client_) client_->clear_windowless_surface();
    if (client_ && client_->browser()) client_->browser()->GetHost()->CloseBrowser(true);
    attached_ = false;
    surface_.reset();
  }

  void resize_surface(const NativeEngineSurface& surface) override {
    surface_ = surface;
    if (client_ && surface.window_handle == 0 && surface.frame_sink) {
      client_->configure_windowless_surface(
          surface.frame_sink, surface.cursor_sink, surface.context_menu_sink,
          surface.text_input_geometry_sink, surface.width, surface.height,
          surface.scale_factor);
    }
    if (client_ && client_->browser()) client_->browser()->GetHost()->WasResized();
  }

  [[nodiscard]] NavigationState navigation_state() const override {
    std::scoped_lock lock(state_mutex_);
    return state_;
  }

  [[nodiscard]] RendererHealth renderer_health() const override {
    return closed_ ? RendererHealth::Terminated : RendererHealth::Healthy;
  }

 private:
  [[nodiscard]] bool windowless_browser() const {
    return client_ && client_->browser() && surface_ &&
           surface_->window_handle == 0 && surface_->frame_sink != nullptr;
  }

  CefRefPtr<CefRequestContext> request_context_;
  EngineViewOptions options_;
  CefRefPtr<GoreeCloudCefClient> client_;
  mutable std::mutex state_mutex_;
  NavigationState state_;
  std::optional<NativeEngineSurface> surface_;
  bool attached_{false};
  bool closed_{false};
};

class CefRuntimeContext final : public ChromiumRuntimeContext {
 public:
  CefRuntimeContext(EngineContextOptions options,
                    CefRefPtr<CefRequestContext> request_context)
      : options_(std::move(options)), request_context_(std::move(request_context)) {}

  [[nodiscard]] std::unique_ptr<ChromiumRuntimeView> create_view(
      const EngineViewOptions& options) override {
    return std::make_unique<CefRuntimeView>(request_context_, options);
  }

  bool clear_origin_data(std::string_view, EngineDataClasses) override { return false; }

  bool clear_all_data(EngineDataClasses classes) override {
    if ((classes & data_class(EngineDataClass::HttpCache)) != 0) {
#if CEF_API_ADDED(14400)
      request_context_->ClearHttpCache(nullptr);
#endif
    }
    return true;
  }

  bool clear_authentication_state(std::optional<std::string_view>) override {
    request_context_->ClearHttpAuthCredentials(nullptr);
    return true;
  }

  bool clear_permission_state(std::optional<std::string_view>) override { return false; }

 private:
  EngineContextOptions options_;
  CefRefPtr<CefRequestContext> request_context_;
};

#endif

class CefRuntimeDelegateScaffold final : public ChromiumRuntimeDelegate {
 public:
  explicit CefRuntimeDelegateScaffold(CefRuntimeOptions options)
      : options_(std::move(options)) {}

  void initialize() override {
    if (initialized_) return;
    if (options_.root.empty()) throw std::runtime_error("CEF runtime root is not configured");
    if (!options_.enable_sandbox) throw std::runtime_error("GoreeCloud Browser refuses to initialize CEF with sandboxing disabled");

#if GOREECLOUD_ENABLE_CEF
    if (options_.process_argc <= 0 || options_.process_argv == nullptr) {
      throw std::runtime_error(
          "CEF browser-process initialization requires the host argc/argv on Linux");
    }
    CefMainArgs main_args(options_.process_argc, options_.process_argv);
    CefSettings settings;
    // Current CEF uses the Chrome bootstrap. Individual embedded child
    // windows select Alloy runtime style for the custom GTK/X11 parent.
    settings.no_sandbox = options_.enable_sandbox ? 0 : 1;
    settings.external_message_pump = options_.external_message_pump ? 1 : 0;
    settings.windowless_rendering_enabled = options_.windowless_rendering ? 1 : 0;
    if (!options_.subprocess_path.empty()) {
      CefString(&settings.browser_subprocess_path) = options_.subprocess_path.string();
    }
    CefString(&settings.resources_dir_path) = options_.resources_path.string();
    CefString(&settings.locales_dir_path) = options_.locales_path.string();
    CefString(&settings.root_cache_path) = options_.cache_root.string();
    CefString(&settings.locale) = options_.locale;

    CefRefPtr<CefApp> app = create_goreecloud_cef_browser_app();
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] entering CefInitialize" << std::endl;
    }
    if (!CefInitialize(main_args, settings, app, nullptr)) {
      throw std::runtime_error("CEF initialization failed");
    }
    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] CefInitialize completed" << std::endl;
    }
#endif
    initialized_ = true;
  }

  void shutdown() override {
    if (!initialized_) return;
#if GOREECLOUD_ENABLE_CEF
    CefShutdown();
#endif
    initialized_ = false;
  }

  [[nodiscard]] std::unique_ptr<ChromiumRuntimeContext> create_context(
      const EngineContextOptions& options) override {
    if (!initialized_) throw std::logic_error("CEF runtime must be initialized before creating contexts");
    if (options.private_context && options.persistent_storage) {
      throw std::invalid_argument("Private CEF contexts must be ephemeral");
    }

#if GOREECLOUD_ENABLE_CEF
    CefRequestContextSettings settings;
    if (!options.private_context && options.persistent_storage) {
      auto storage_path = std::filesystem::path{options.storage_path};
      if (storage_path.is_relative()) {
        storage_path = options_.cache_root / storage_path;
      }
      CefString(&settings.cache_path) = storage_path.lexically_normal().string();
    }
    CefString(&settings.accept_language_list) = options.locale;
    auto request_context = CefRequestContext::CreateContext(settings, nullptr);
    if (!request_context) throw std::runtime_error("CEF failed to create request context");
    return std::make_unique<CefRuntimeContext>(options, std::move(request_context));
#else
    return nullptr;
#endif
  }

  void do_message_loop_work() override {
    if (!initialized_) return;
#if GOREECLOUD_ENABLE_CEF
    CefDoMessageLoopWork();
#endif
  }

  [[nodiscard]] std::string_view runtime_version() const noexcept override {
    return "cef-runtime-client-context-media-preview";
  }

 private:
  CefRuntimeOptions options_;
  bool initialized_{false};
};

}  // namespace

std::unique_ptr<ChromiumRuntimeDelegate> create_cef_runtime_delegate(CefRuntimeOptions options) {
  return std::make_unique<CefRuntimeDelegateScaffold>(std::move(options));
}

}  // namespace goreecloud::browser
