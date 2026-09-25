#include "goreecloud/browser/chromium_runtime_factory.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#if GOREECLOUD_ENABLE_CEF
#include "goreecloud/browser/cef_runtime.hpp"
#endif

namespace goreecloud::browser {
namespace {

class ScaffoldRuntimeView final : public ChromiumRuntimeView {
 public:
  explicit ScaffoldRuntimeView(EngineViewOptions options)
      : allow_popups_(options.allow_popups) {
    navigate(options.initial_url);
  }

  void navigate(std::string_view url) override {
    if (history_index_ + 1 < history_.size()) {
      history_.erase(history_.begin() + static_cast<std::ptrdiff_t>(history_index_ + 1),
                     history_.end());
    }
    history_.push_back(std::string{url});
    history_index_ = history_.size() - 1;
    sync_history();
  }

  void reload(bool bypass_cache) override {
    bypass_cache_last_reload_ = bypass_cache;
    navigation_.loading = false;
    navigation_.progress = 1.0;
  }

  void stop() override { navigation_.loading = false; }

  void go_back() override {
    if (history_index_ == 0) return;
    --history_index_;
    sync_history();
  }

  void go_forward() override {
    if (history_index_ + 1 >= history_.size()) return;
    ++history_index_;
    sync_history();
  }

  void set_zoom(double factor) override {
    zoom_factor_ = std::clamp(factor, 0.25, 5.0);
  }

  void find(std::string_view query, bool forward) override {
    last_find_query_ = std::string{query};
    last_find_forward_ = forward;
  }

  void stop_find() override { last_find_query_.clear(); }

  bool attach_surface(const NativeEngineSurface& surface) override {
    if (surface.width <= 0 || surface.height <= 0) return false;
    surface_ = surface;
    return true;
  }

  void detach_surface() override { surface_.reset(); }

  void resize_surface(const NativeEngineSurface& surface) override {
    if (!surface_) return;
    surface_ = surface;
  }

  [[nodiscard]] NavigationState navigation_state() const override {
    return navigation_;
  }

  [[nodiscard]] RendererHealth renderer_health() const override {
    return RendererHealth::Healthy;
  }

 private:
  void sync_history() {
    navigation_.url = history_[history_index_];
    navigation_.title = navigation_.url;
    navigation_.loading = false;
    navigation_.progress = 1.0;
    navigation_.can_go_back = history_index_ > 0;
    navigation_.can_go_forward = history_index_ + 1 < history_.size();
  }

  NavigationState navigation_;
  std::vector<std::string> history_;
  std::size_t history_index_{0};
  double zoom_factor_{1.0};
  std::string last_find_query_;
  bool last_find_forward_{true};
  bool bypass_cache_last_reload_{false};
  bool allow_popups_{false};
  std::optional<NativeEngineSurface> surface_;
};

class ScaffoldRuntimeContext final : public ChromiumRuntimeContext {
 public:
  explicit ScaffoldRuntimeContext(EngineContextOptions options)
      : options_(std::move(options)) {}

  [[nodiscard]] std::unique_ptr<ChromiumRuntimeView> create_view(
      const EngineViewOptions& options) override {
    return std::make_unique<ScaffoldRuntimeView>(options);
  }

  bool clear_origin_data(std::string_view origin,
                         EngineDataClasses classes) override {
    cleared_origin_classes_[std::string{origin}] |= classes;
    return true;
  }

  bool clear_all_data(EngineDataClasses classes) override {
    cleared_all_classes_ |= classes;
    return true;
  }

  bool clear_authentication_state(std::optional<std::string_view> origin) override {
    if (origin) cleared_auth_origins_.push_back(std::string{*origin});
    else cleared_all_authentication_ = true;
    return true;
  }

  bool clear_permission_state(std::optional<std::string_view> origin) override {
    if (origin) cleared_permission_origins_.push_back(std::string{*origin});
    else cleared_all_permissions_ = true;
    return true;
  }

 private:
  EngineContextOptions options_;
  EngineDataClasses cleared_all_classes_{0};
  std::unordered_map<std::string, EngineDataClasses> cleared_origin_classes_;
  std::vector<std::string> cleared_auth_origins_;
  std::vector<std::string> cleared_permission_origins_;
  bool cleared_all_authentication_{false};
  bool cleared_all_permissions_{false};
};

class ScaffoldRuntimeDelegate final : public ChromiumRuntimeDelegate {
 public:
  explicit ScaffoldRuntimeDelegate(ChromiumAdapterOptions options)
      : options_(std::move(options)) {}

  void initialize() override {
    if (initialized_) return;
    if (options_.runtime_root.empty()) {
      throw std::runtime_error("Chromium runtime root is not configured");
    }
    initialized_ = true;
  }

  void shutdown() override { initialized_ = false; }

  [[nodiscard]] std::unique_ptr<ChromiumRuntimeContext> create_context(
      const EngineContextOptions& options) override {
    if (!initialized_) {
      throw std::logic_error("Chromium runtime must be initialized before creating contexts");
    }
    return std::make_unique<ScaffoldRuntimeContext>(options);
  }

  void do_message_loop_work() override {}

  [[nodiscard]] std::string_view runtime_version() const noexcept override {
    return "runtime-delegate-scaffold";
  }

 private:
  ChromiumAdapterOptions options_;
  bool initialized_{false};
};

}  // namespace

std::unique_ptr<ChromiumRuntimeDelegate> create_chromium_runtime_delegate(
    const ChromiumAdapterOptions& options) {
#if GOREECLOUD_ENABLE_CEF
  CefRuntimeOptions cef;
  cef.root = options.runtime_root;
  cef.subprocess_path = options.subprocess_path;
  cef.resources_path = options.resources_path;
  cef.locales_path = options.locales_path;
  cef.cache_root = options.runtime_root / "profile-cache";
  cef.process_argc = options.process_argc;
  cef.process_argv = options.process_argv;
  cef.enable_gpu = options.enable_gpu;
  cef.enable_sandbox = options.enable_sandbox;
  cef.external_message_pump = true;
  return create_cef_runtime_delegate(std::move(cef));
#else
  return std::make_unique<ScaffoldRuntimeDelegate>(options);
#endif
}

}  // namespace goreecloud::browser
