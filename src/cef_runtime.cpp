#include "goreecloud/browser/cef_runtime.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>
#include <stdexcept>
#include <utility>

#if GOREECLOUD_ENABLE_CEF
#include "goreecloud/browser/cef_browser_app.hpp"
#include "goreecloud/browser/cef_client.hpp"
#include "goreecloud/browser/cef_media_probe_app.hpp"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_request_context.h"
#endif

#if defined(OS_LINUX)
#include <X11/Xlib.h>
#endif

namespace goreecloud::browser {

namespace {

#if GOREECLOUD_ENABLE_CEF

bool cef_runtime_diagnostics_enabled() {
  const char* value = std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
  return value && *value && std::string_view{value} != "0";
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
    surface_ = surface;
    if (client_ && client_->browser()) {
      attached_ = true;
      client_->browser()->GetHost()->WasResized();
      return true;
    }
    if (!client_) {
      client_ = new GoreeCloudCefClient(
          [this](const NavigationState& state) {
            std::scoped_lock lock(state_mutex_);
            state_ = state;
          },
          [this]() { closed_ = true; },
          {},
          [this](CefRefPtr<CefBrowser> browser) {
            show_native_child(browser);
          });
    }

    CefWindowInfo window_info;
    window_info.SetAsChild(static_cast<CefWindowHandle>(surface.window_handle),
                           CefRect(surface.x, surface.y, surface.width, surface.height));
    // CEF 128+ uses the Chrome bootstrap. GoreeCloud embeds the browser into
    // its own GTK/X11 parent, so the child must explicitly use Alloy runtime
    // style while retaining the current Chrome bootstrap.
    window_info.runtime_style = CEF_RUNTIME_STYLE_ALLOY;

    std::string initial_url;
    {
      std::scoped_lock lock(state_mutex_);
      initial_url = state_.url.empty() ? options_.initial_url : state_.url;
    }

    CefBrowserSettings browser_settings;
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
                << " url=" << initial_url
                << " parent=" << surface.window_handle
                << " size=" << surface.width << "x" << surface.height
                << std::endl;
    }
    attached_ = created;
    return created;
  }

  void detach_surface() override {
    if (client_ && client_->browser()) client_->browser()->GetHost()->CloseBrowser(true);
    attached_ = false;
    surface_.reset();
  }

  void resize_surface(const NativeEngineSurface& surface) override {
    surface_ = surface;
#if defined(OS_LINUX)
    if (client_ && client_->browser()) {
#if defined(CEF_X11)
      auto* display = cef_get_xdisplay();
#else
      auto* display = reinterpret_cast<Display*>(surface.display_handle);
#endif
      const auto xwindow =
          static_cast<::Window>(client_->browser()->GetHost()->GetWindowHandle());
      if (display && xwindow != 0) {
        XMoveResizeWindow(display, xwindow,
                          surface.x, surface.y,
                          static_cast<unsigned int>(surface.width),
                          static_cast<unsigned int>(surface.height));
        XFlush(display);
      }
    }
#endif
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
  void show_native_child(CefRefPtr<CefBrowser> browser) {
#if defined(OS_LINUX)
    if (!browser || !surface_) return;
#if defined(CEF_X11)
    auto* display = cef_get_xdisplay();
#else
    auto* display = reinterpret_cast<Display*>(surface_->display_handle);
#endif
    const auto xwindow =
        static_cast<::Window>(browser->GetHost()->GetWindowHandle());
    if (!display || xwindow == 0) return;

    const auto parent_window =
        static_cast<::Window>(surface_->window_handle);

    ::Window root_return = 0;
    ::Window current_parent = 0;
    ::Window* children = nullptr;
    unsigned int child_count = 0;
    if (parent_window != 0 &&
        XQueryTree(display, xwindow, &root_return, &current_parent,
                   &children, &child_count)) {
      if (children) XFree(children);
      if (current_parent != parent_window) {
        XReparentWindow(display, xwindow, parent_window,
                        surface_->x, surface_->y);
      }
    }

    XMoveResizeWindow(display, xwindow,
                      surface_->x, surface_->y,
                      static_cast<unsigned int>(surface_->width),
                      static_cast<unsigned int>(surface_->height));

    const char* atom_names[] = {
        "_NET_WM_STATE", "ATOM", "_NET_WM_STATE_HIDDEN"};
    Atom atoms[3]{};
    if (XInternAtoms(display,
                     const_cast<char**>(atom_names),
                     3, False, atoms)) {
      XChangeProperty(display, xwindow,
                      atoms[0], atoms[1], 32,
                      PropModeReplace, nullptr, 0);
    }

    XMapWindow(display, xwindow);
    XSync(display, False);
    browser->GetHost()->WasResized();

    ::Window verified_root = 0;
    ::Window verified_parent = 0;
    ::Window* verified_children = nullptr;
    unsigned int verified_child_count = 0;
    const bool verified_tree =
        XQueryTree(display, xwindow, &verified_root, &verified_parent,
                   &verified_children, &verified_child_count) != 0;
    if (verified_children) XFree(verified_children);

    if (cef_runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] native-child-shown window="
                << static_cast<std::uintptr_t>(xwindow)
                << " display=cef"
                << " parent="
                << static_cast<std::uintptr_t>(
                       verified_tree ? verified_parent : 0)
                << " expected_parent="
                << surface_->window_handle
                << " size=" << surface_->width << "x" << surface_->height
                << std::endl;
    }
#else
    (void)browser;
#endif
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
      CefString(&settings.cache_path) =
          storage_path.lexically_normal().string();
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
