#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "goreecloud/browser/engine.hpp"
#include "goreecloud/browser/in_memory_tab_manager.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/normal_session_runtime.hpp"
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
                     BrowserApplicationOptions options = {},
                     NormalSessionRuntimeCoordinator* session_runtime = nullptr)
      : engine_(std::move(engine)),
        options_(std::move(options)),
        session_runtime_(session_runtime) {
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
    if (normal_session_started_ && session_runtime_) {
      (void)session_runtime_->clean_shutdown();
    }
    windows_.clear();
    tab_manager_ = InMemoryAdvancedTabManager{};
    private_contexts_.clear();
    default_context_.reset();
    engine_->shutdown();
    initialized_ = false;
    normal_session_started_ = false;
    normal_session_attempted_ = false;
  }

  [[nodiscard]] WindowController& new_window(bool private_window) {
    if (private_window) {
      return new_private_window("shared-private");
    }
    require_initialized();
    if (!ensure_default_context()) {
      throw std::runtime_error("Default Browser context unavailable");
    }
    ensure_normal_session_started();
    const auto window_id = next_window_id();
    auto* observer =
        normal_session_started_ && session_runtime_ &&
                session_runtime_->accepting_runtime_events()
            ? static_cast<NormalSessionRuntimeObserver*>(session_runtime_)
            : nullptr;
    windows_.push_back(std::make_unique<WindowController>(
        *default_context_, false, &tab_manager_, window_id, std::string{}, observer));
    if (observer) {
      (void)observer->normal_window_opened(window_id);
    }
    return *windows_.back();
  }

  [[nodiscard]] WindowController& new_private_window(const std::string& private_session_id) {
    require_initialized();
    auto* context = ensure_private_context(private_session_id);
    if (!context) throw std::runtime_error("Private Browser context unavailable");
    windows_.push_back(std::make_unique<WindowController>(
        *context, true, nullptr, next_window_id(), private_session_id, nullptr));
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

  bool close_private_session(const std::string& private_session_id) {
    require_initialized();
    const auto found = private_contexts_.find(private_session_id);
    if (found == private_contexts_.end()) return false;

    windows_.erase(
        std::remove_if(
            windows_.begin(), windows_.end(),
            [&private_session_id](const auto& window) {
              return window && window->private_window() &&
                     window->private_session_id() == private_session_id;
            }),
        windows_.end());

    // Private contexts are created without persistent storage. Destroy every
    // Browser-owned view first, then destroy the exact ephemeral context so no
    // WindowController can retain a dangling EngineContext reference.
    private_contexts_.erase(found);
    return true;
  }

  bool destroy_private_session_context(const std::string& private_session_id) {
    return close_private_session(private_session_id);
  }

  [[nodiscard]] bool has_private_session_context(const std::string& private_session_id) const {
    return private_contexts_.contains(private_session_id);
  }

  [[nodiscard]] bool has_default_context() const noexcept {
    return static_cast<bool>(default_context_);
  }

  [[nodiscard]] bool close_window(std::string_view window_id) {
    require_initialized();
    const auto found = std::find_if(
        windows_.begin(), windows_.end(),
        [window_id](const auto& window) {
          return window && window->window_id() == window_id;
        });
    if (found == windows_.end()) return false;

    const bool normal_window = !(*found)->private_window();
    const std::string durable_id{(*found)->window_id()};
    if (normal_window && normal_session_started_ && session_runtime_) {
      if (!session_runtime_->accepting_runtime_events() ||
          !session_runtime_->normal_window_closed(durable_id)) {
        return false;
      }
    }
    windows_.erase(found);
    return true;
  }

  void background_normal_session() {
    if (normal_session_started_ && session_runtime_ &&
        session_runtime_->accepting_runtime_events()) {
      (void)session_runtime_->background();
    }
  }

  void resume_normal_session() {
    if (normal_session_started_ && session_runtime_ &&
        session_runtime_->accepting_runtime_events()) {
      (void)session_runtime_->resume();
    }
  }

  void pump_events() {
    require_initialized();
    engine_->pump_events();
    for (const auto& window : windows_) {
      if (window && !window->private_window()) {
        window->sync_navigation_metadata();
      }
    }
  }

  [[nodiscard]] const NormalSessionRuntimeStatus* normal_session_status() const noexcept {
    return session_runtime_ ? &session_runtime_->status() : nullptr;
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
  void ensure_normal_session_started() {
    if (!session_runtime_ || normal_session_attempted_) return;
    normal_session_attempted_ = true;
    normal_session_started_ = session_runtime_->begin();
  }

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

  std::string next_window_id() {
    return "window-" + std::to_string(next_window_id_++);
  }

  std::unique_ptr<BrowserEngine> engine_;
  BrowserApplicationOptions options_;
  std::unique_ptr<EngineContext> default_context_;
  std::unordered_map<std::string, std::unique_ptr<EngineContext>> private_contexts_;
  InMemoryAdvancedTabManager tab_manager_;
  std::vector<std::unique_ptr<WindowController>> windows_;
  NormalSessionRuntimeCoordinator* session_runtime_{nullptr};
  std::uint64_t next_window_id_{1};
  bool initialized_{false};
  bool normal_session_attempted_{false};
  bool normal_session_started_{false};
};

}  // namespace goreecloud::browser
