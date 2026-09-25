#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>
#include <string>

#include "goreecloud/browser/application.hpp"
#include "goreecloud/browser/cef_process.hpp"
#include "goreecloud/browser/command_line.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/runtime_engine_factory.hpp"
#include "goreecloud/browser/toolbar.hpp"
#include "goreecloud/browser/unified_search_bar.hpp"
#include "goreecloud/browser/version.hpp"

#if GOREECLOUD_ENABLE_LINUX_GTK_HOST
#include "goreecloud/browser/platform/gtk_linux_runner.hpp"
#endif

int main(int argc, char** argv) {
  using namespace goreecloud::browser;

  const int cef_exit_code = execute_cef_subprocess_if_needed(argc, argv);
  if (cef_exit_code >= 0) return cef_exit_code;

  try {
    const auto launch = parse_browser_launch_request(argc, argv);

    BrowserApplicationOptions options;
    options.initial_private_window = launch.private_window;
    options.initial_private_session_id = launch.isolated_private_window
                                             ? "isolated-private-1"
                                             : "shared-private";
    if (!launch.urls.empty()) {
      options.initial_url = launch.urls.front();
    } else {
      options.initial_url = launch.private_window
                                ? std::string{kPrivateStartUrl}
                                : std::string{kNewTabUrl};
    }

    const char* diagnostics =
        std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
    const bool diagnostic =
        diagnostics && *diagnostics && std::string_view{diagnostics} != "0";
    if (diagnostic) {
      std::cerr << "[GoreeCloud CEF] selecting runtime engine" << std::endl;
    }

    auto selection = create_runtime_engine_from_environment(argc, argv);
    if (diagnostic) {
      std::cerr << "[GoreeCloud CEF] selected runtime mode="
                << selection.mode << std::endl;
    }

    BrowserApplication browser(std::move(selection.engine), options);
    if (diagnostic) {
      std::cerr << "[GoreeCloud CEF] initializing BrowserApplication" << std::endl;
    }
    browser.initialize();
    if (diagnostic) {
      std::cerr << "[GoreeCloud CEF] BrowserApplication initialized" << std::endl;
    }

    if (auto* first_window = browser.first_window()) {
      for (std::size_t index = 1; index < launch.urls.size(); ++index) {
        (void)first_window->new_tab(launch.urls[index]);
      }
    }

    std::cout << "GoreeCloud Browser " << kBrowserVersion << "\n";
    std::cout << "Release channel: " << kBrowserReleaseChannel << "\n";
    std::cout << "Runtime mode: " << selection.mode << "\n";
    std::cout << "Engine: " << browser.engine().name() << " "
              << browser.engine().version() << "\n";
    std::cout << "Windows: " << browser.window_count() << "\n";
    std::cout << "Private startup: " << (launch.private_window ? "yes" : "no") << "\n";
    std::cout << "Startup surface: " << options.initial_url << "\n";
    std::cout << "Toolbar controls: " << kDefaultToolbar.size() << "\n";
    std::cout << "Search authority: " << kSearchAuthority << "\n";
    std::cout << "Glaze UI: approved Stable baseline enforced by build\n";
    std::cout << "Production renderer requested: "
              << (selection.production_renderer_requested ? "yes" : "no") << "\n";
    std::cout << "Production approved: " << (kProductionApproved ? "yes" : "no") << "\n";

#if GOREECLOUD_ENABLE_LINUX_GTK_HOST
    const int result = platform::run_gtk_linux_browser(browser);
    browser.shutdown();
    return result;
#else
    browser.shutdown();
    return 0;
#endif
  } catch (const std::exception& error) {
    std::cerr << "GoreeCloud Browser startup failed: " << error.what() << "\n";
    return 1;
  }
}
