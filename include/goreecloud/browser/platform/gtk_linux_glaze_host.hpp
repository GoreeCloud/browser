#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "goreecloud/browser/media_hover.hpp"
#include "goreecloud/browser/media_preview_provider.hpp"
#include "goreecloud/browser/native_window_host.hpp"
#include "goreecloud/browser/toolbar.hpp"

namespace goreecloud::browser::platform {

enum class GtkTabAction {
  activate,
  close,
  create,
  next,
  previous,
};

class GtkLinuxGlazeWindowHost final : public NativeWindowHost {
 public:
  using ToolbarHandler = std::function<void(ToolbarItem)>;
  using TabActionHandler = std::function<void(GtkTabAction, std::string_view)>;
  using SearchHandler = std::function<void(std::string_view)>;
  using SearchControlHandler = std::function<void(UnifiedSearchBarControl)>;
  using MediaHoverActionHandler = std::function<void(MediaAction, const MediaTarget&)>;

  GtkLinuxGlazeWindowHost();
  ~GtkLinuxGlazeWindowHost() override;

  GtkLinuxGlazeWindowHost(const GtkLinuxGlazeWindowHost&) = delete;
  GtkLinuxGlazeWindowHost& operator=(const GtkLinuxGlazeWindowHost&) = delete;

  void set_toolbar_handler(ToolbarHandler handler);
  void set_tab_action_handler(TabActionHandler handler);
  void set_search_handler(SearchHandler handler);
  void set_search_control_handler(SearchControlHandler handler);
  void set_media_hover_action_handler(MediaHoverActionHandler handler);
  void set_media_hover_policy(MediaHoverSitePolicy policy);
  void set_private_window(bool private_window);

  bool create() override;
  void show() override;
  void close() override;
  void set_title(std::string_view title) override;
  void render_chrome(const BrowserChromeState& state) override;
  void attach_engine_view(EngineView& view) override;
  void detach_engine_view() override;
  void show_internal_surface(std::string_view internal_url) override;
  void show_panel(std::string_view panel_id) override;
  [[nodiscard]] NativeWindowMetrics metrics() const override;

  [[nodiscard]] bool copy_text_to_clipboard(std::string_view text);
  [[nodiscard]] std::optional<MediaSaveDestination> choose_media_save_destination(MediaKind kind);
  [[nodiscard]] bool confirm_media_boundary(std::string_view title,
                                            std::string_view explanation);
  [[nodiscard]] bool show_media_preview(const MediaPreviewFrame& frame,
                                        std::string_view accessible_description);
  void show_media_action_status(std::string_view message);

  bool pump_events();
  [[nodiscard]] bool close_requested() const noexcept;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace goreecloud::browser::platform
