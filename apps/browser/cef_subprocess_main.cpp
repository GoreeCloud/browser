#include <cstdlib>
#include <iostream>
#include <string_view>

#include "goreecloud/browser/cef_process.hpp"

int main(int argc, char** argv) {
  const int exit_code =
      goreecloud::browser::execute_cef_subprocess_if_needed(argc, argv);

  if (exit_code >= 0) return exit_code;

  const char* diagnostics =
      std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
  if (diagnostics && *diagnostics && std::string_view{diagnostics} != "0") {
    std::cerr
        << "[GoreeCloud CEF] subprocess helper invoked without a CEF process type"
        << std::endl;
  }
  return 1;
}
