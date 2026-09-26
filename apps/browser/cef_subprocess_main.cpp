#include <cstdlib>
#include <iostream>
#include <string_view>

#include "goreecloud/browser/cef_media_probe_app.hpp"
#include "include/cef_app.h"

namespace {

bool is_renderer_process(int argc, char** argv) {
  for (int index = 1; index < argc; ++index) {
    if (!argv[index]) continue;
    if (std::string_view{argv[index]} == "--type=renderer") return true;
  }
  return false;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace goreecloud::browser;

  const char* diagnostics =
      std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
  const bool diagnostic =
      diagnostics && *diagnostics && std::string_view{diagnostics} != "0";

  const bool renderer = is_renderer_process(argc, argv);
  CefRefPtr<CefApp> app;
  if (renderer) {
    app = new GoreeCloudCefRenderApp();
  }

  if (diagnostic) {
    std::cerr << "[GoreeCloud CEF] subprocess role="
              << (renderer ? "renderer" : "default") << std::endl;
  }

  CefMainArgs main_args(argc, argv);
  const int exit_code = CefExecuteProcess(main_args, app, nullptr);

  if (diagnostic) {
    std::cerr << "[GoreeCloud CEF] subprocess exit=" << exit_code << std::endl;
  }

  return exit_code >= 0 ? exit_code : 1;
}
