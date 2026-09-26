#pragma once

#include "goreecloud/browser/cef_media_probe_app.hpp"

namespace goreecloud::browser {

#if GOREECLOUD_ENABLE_CEF

inline CefRefPtr<GoreeCloudCefBrowserApp> goreecloud_cef_browser_app() {
  static CefRefPtr<GoreeCloudCefBrowserApp> app = new GoreeCloudCefBrowserApp();
  return app;
}

#endif

}  // namespace goreecloud::browser
