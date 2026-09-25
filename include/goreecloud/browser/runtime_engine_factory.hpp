#pragma once

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "goreecloud/browser/development_engine.hpp"
#include "goreecloud/browser/engine.hpp"

#if GOREECLOUD_ENABLE_CHROMIUM
#include "goreecloud/browser/chromium_adapter.hpp"
#endif

namespace goreecloud::browser {

struct RuntimeEngineSelection {
  std::unique_ptr<BrowserEngine> engine;
  bool production_renderer_requested{false};
  std::string mode;
};

inline RuntimeEngineSelection create_runtime_engine_from_environment(
    int process_argc = 0, char** process_argv = nullptr) {
#if GOREECLOUD_ENABLE_CHROMIUM
  const char* runtime_root_env = std::getenv("GOREECLOUD_BROWSER_RUNTIME_ROOT");
  if (!runtime_root_env || std::string{runtime_root_env}.empty()) {
    throw std::runtime_error(
        "GOREECLOUD_BROWSER_RUNTIME_ROOT must point to the Chromium/CEF runtime when Chromium mode is enabled");
  }

  ChromiumAdapterOptions options;
  options.runtime_root = std::filesystem::path{runtime_root_env};
  options.process_argc = process_argc;
  options.process_argv = process_argv;

  if (const char* subprocess = std::getenv("GOREECLOUD_BROWSER_SUBPROCESS");
      subprocess && std::string{subprocess}.size() > 0) {
    options.subprocess_path = std::filesystem::path{subprocess};
  } else {
    const auto bundled_subprocess =
        options.runtime_root / "goreecloud-browser-subprocess";
    if (std::filesystem::exists(bundled_subprocess)) {
      options.subprocess_path = bundled_subprocess;
    }
  }
  if (const char* resources = std::getenv("GOREECLOUD_BROWSER_RESOURCES")) {
    options.resources_path = std::filesystem::path{resources};
  } else {
    options.resources_path = options.runtime_root;
  }
  if (const char* locales = std::getenv("GOREECLOUD_BROWSER_LOCALES")) {
    options.locales_path = std::filesystem::path{locales};
  } else {
    options.locales_path = options.runtime_root / "locales";
  }

  return {std::make_unique<ChromiumEngineAdapter>(std::move(options)), true,
#if GOREECLOUD_ENABLE_CEF
          "cef"
#else
          "chromium-scaffold"
#endif
  };
#else
  return {std::make_unique<DevelopmentEngine>(), false, "development"};
#endif
}

}  // namespace goreecloud::browser
