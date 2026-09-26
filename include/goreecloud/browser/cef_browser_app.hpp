#pragma once

#include "include/cef_app.h"
#include "include/cef_command_line.h"

namespace goreecloud::browser {

#if GOREECLOUD_ENABLE_CEF

class GoreeCloudCefProcessApp final : public CefApp {
 public:
  GoreeCloudCefProcessApp() = default;

  void OnBeforeCommandLineProcessing(
      const CefString& process_type,
      CefRefPtr<CefCommandLine> command_line) override {
    if (process_type.empty() && command_line &&
        !command_line->HasSwitch("no-first-run")) {
      command_line->AppendSwitch("no-first-run");
    }
  }

 private:
  IMPLEMENT_REFCOUNTING(GoreeCloudCefProcessApp);
  DISALLOW_COPY_AND_ASSIGN(GoreeCloudCefProcessApp);
};

inline CefRefPtr<CefApp> create_goreecloud_cef_browser_app() {
  return new GoreeCloudCefProcessApp();
}

#endif

}  // namespace goreecloud::browser
