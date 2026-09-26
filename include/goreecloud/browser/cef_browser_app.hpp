#pragma once

#include <atomic>

#if GOREECLOUD_ENABLE_CEF
#include "include/cef_app.h"
#endif

namespace goreecloud::browser {

#if GOREECLOUD_ENABLE_CEF

class GoreeCloudCefBrowserApp final : public CefApp,
                                      public CefBrowserProcessHandler {
 public:
  GoreeCloudCefBrowserApp() = default;

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnContextInitialized() override {
    browser_context_initialized_.store(true, std::memory_order_release);
  }

  [[nodiscard]] bool browser_context_initialized() const noexcept {
    return browser_context_initialized_.load(std::memory_order_acquire);
  }

 private:
  std::atomic_bool browser_context_initialized_{false};

  IMPLEMENT_REFCOUNTING(GoreeCloudCefBrowserApp);
  DISALLOW_COPY_AND_ASSIGN(GoreeCloudCefBrowserApp);
};

inline CefRefPtr<GoreeCloudCefBrowserApp> goreecloud_cef_browser_app() {
  static CefRefPtr<GoreeCloudCefBrowserApp> app = new GoreeCloudCefBrowserApp();
  return app;
}

#endif

}  // namespace goreecloud::browser
