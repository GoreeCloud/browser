#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "goreecloud/browser/engine.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/private_browsing.hpp"
#include "goreecloud/browser/window_controller.hpp"

namespace goreecloud::browser {

struct BrowserApplicationOptions {
  std::string profile_id{"default"};
  std::string storage_path{"profile"};
  std::string locale{"en-US"};
  std::string initial_url{std::string{kNewTabUrl}};
  bool initial_private_window{false};
  std::string initial_private_session_id{"shared-private"};
};

class BrowserApplication {
 public:
  BrowserApplication(std::unique_ptr<BrowserEngine> engine,
                     BrowserApplicationOptions options = {})
      : engine_(std::move(engine)), options_(std::move(options)) {
    if (!engine_) {
      throw std::invalid_argument("BrowserApplication requires a BrowserEngine");
    }
  }

  ~BrowserApplication() {
    if (initialized_) shutdown();
  }

  BrowserApplication(const BrowserApplication&) = delete;
  BrowserApplication& operator=(const BrowserApplication&) = delete;

  void initialize() {
    if (initialized_) return;
    engine_->initialize();

    if (!options_.initial_private_window && !ensure_default_context()) {
      engine_->shutdown();
      throw std::runtime_error("Browser engine failed to create default context");
    }

    initialized_ = true;
    if (options_.initial_private_window) {
      auto& window = new_private_window(options_.initial_private_session_id);
      (void)window.new_tab(options_.initial_url.empty()
                               ? std::string{kPrivateStartUrl}
                               : options_.initial_url);
    } else {
      auto& window = new_window(false);
      (void)window.new_tab(options_.initial_url.empty()
                               ? std::string{kNewTabUrl}
                               : options_.initial_url);
    }
  }

  void shutdown() noexcept {
    windows_.clear();
    private_contexts_.clear();
    default_context_.reset();
    engine_->shutdown();
    initialized_ = false;
  }

  [[nodiscard]] WindowController& new_window(bool private_window) {
    if (private_window) {
      return new_private_window("shared-private");
    }
    require_initialized();
    if (!ensure_default_context()) {
      throw std::runtime_error("Default Browser context unavailable");
    }
    windows_.push_back(std::make_unique<WindowController>(*default_context_, false));
    return *windows_.back();
  }

  [[nodiscard]] WindowController& new_private_window(const std::string& private_session_id) {
    require_initialized();
    auto* context = ensure_private_context(private_session_id);
    if (!context) throw std::runtime_error("Private Browser context unavailable");
    windows_.push_back(std::make_unique<WindowController>(*context, true));
    return *windows_.back();
  }

  [[nodiscard]] EngineContext* private_session_context(const std::string& private_session_id) {
    require_initialized();
    const auto found = private_contexts_.find(private_session_id);
    return found == private_contexts_.end() ? nullptr : found->second.get();
  }

  [[nodiscard]] const EngineContext* private_session_context(const std::string& private_session_id) const {
    const auto found = private_contexts_.find(private_session_id);
    return found == private_contexts_.end() ? nullptr : found->second.get();
  }

  bool destroy_private_session_context(const std::string& private_session_id) {
    require_initialized();
    return private_contexts_.erase(private_session_id) > 0;
  }

  [[nodiscard]] bool has_private_session_context(const std::string& private_session_id) const {
    return private_contexts_.contains(private_session_id);
  }

  [[nodiscard]] BrowserEngine& engine() noexcept { return *engine_; }
  [[nodiscard]] const BrowserEngine& engine() const noexcept { return *engine_; }
  [[nodiscard]] std::size_t window_count() const noexcept { return windows_.size(); }
  [[nodiscard]] bool initialized() const noexcept { return initialized_; }

  [[nodiscard]] WindowController* first_window() noexcept {
    return windows_.empty() ? nullptr : windows_.front().get();
  }

  [[nodiscard]] const WindowController* first_window() const noexcept {
    return windows_.empty() ? nullptr : windows_.front().get();
  }

 private:
  bool ensure_default_context() {
    if (default_context_) return true;

    EngineContextOptions context_options;
    context_options.profile_id = options_.profile_id;
    context_options.storage_path = options_.storage_path;
    context_options.locale = options_.locale;
    context_options.private_context = false;
    context_options.persistent_storage = true;

    default_context_ = engine_->create_context(context_options);
    return static_cast<bool>(default_context_);
  }

  EngineContext* ensure_private_context(const std::string& private_session_id) {
    const auto found = private_contexts_.find(private_session_id);
    if (found != private_contexts_.end()) return found->second.get();

    EngineContextOptions private_options;
    private_options.profile_id = options_.profile_id + "-private-" + private_session_id;
    private_options.storage_path.clear();
    private_options.locale = options_.locale;
    private_options.private_context = true;
    private_options.persistent_storage = false;

    auto context = engine_->create_context(private_options);
    if (!context) return nullptr;
    auto* raw = context.get();
    private_contexts_.emplace(private_session_id, std::move(context));
    return raw;
  }

  void require_initialized() const {
    if (!initialized_) {
      throw std::logic_error("BrowserApplication is not initialized");
    }
  }

  std::unique_ptr<BrowserEngine> engine_;
  BrowserApplicationOptions options_;
  std::unique_ptr<EngineContext> default_context_;
  std::unordered_map<std::string, std::unique_ptr<EngineContext>> private_contexts_;
  std::vector<std::unique_ptr<WindowController>> windows_;
  bool initialized_{false};
};

}  // namespace goreecloud::browser
