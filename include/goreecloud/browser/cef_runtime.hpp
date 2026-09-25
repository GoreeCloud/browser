#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "goreecloud/browser/chromium_runtime_delegate.hpp"

namespace goreecloud::browser {

struct CefRuntimeOptions {
  std::filesystem::path root;
  std::filesystem::path subprocess_path;
  std::filesystem::path resources_path;
  std::filesystem::path locales_path;
  std::filesystem::path cache_root;
  std::string locale{"en-US"};
  int process_argc{0};
  char** process_argv{nullptr};
  bool enable_gpu{true};
  bool enable_sandbox{true};
  bool external_message_pump{false};
  bool windowless_rendering{false};
};

[[nodiscard]] std::unique_ptr<ChromiumRuntimeDelegate> create_cef_runtime_delegate(
    CefRuntimeOptions options);

}  // namespace goreecloud::browser
