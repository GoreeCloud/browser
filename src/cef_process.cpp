#include "goreecloud/browser/cef_process.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

#if GOREECLOUD_ENABLE_CEF
#include "goreecloud/browser/cef_browser_app.hpp"
#include "goreecloud/browser/cef_media_probe_app.hpp"
#include "include/cef_app.h"
#endif

namespace goreecloud::browser {

int execute_cef_subprocess_if_needed(int argc, char** argv) {
#if GOREECLOUD_ENABLE_CEF
  const char* diagnostics =
      std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
  const bool diagnostic =
      diagnostics && *diagnostics && std::string_view{diagnostics} != "0";
  if (diagnostic) {
    std::cerr << "[GoreeCloud CEF] entering CefExecuteProcess" << std::endl;
  }

  CefMainArgs main_args(argc, argv);

  bool has_process_type = false;
  bool renderer_process = false;
  for (int index = 1; index < argc; ++index) {
    if (!argv[index]) continue;
    const std::string_view argument{argv[index]};
    if (argument.rfind("--type=", 0) == 0) {
      has_process_type = true;
      renderer_process = argument == "--type=renderer";
      break;
    }
  }

  CefRefPtr<CefApp> app;
  if (renderer_process) {
    app = new GoreeCloudCefRenderApp();
  } else if (!has_process_type) {
    app = goreecloud_cef_browser_app();
  }

  const int exit_code = CefExecuteProcess(main_args, app, nullptr);

  if (diagnostic) {
    std::cerr << "[GoreeCloud CEF] CefExecuteProcess returned "
              << exit_code << std::endl;
  }
  return exit_code;
#else
  (void)argc;
  (void)argv;
  return -1;
#endif
}

}  // namespace goreecloud::browser
