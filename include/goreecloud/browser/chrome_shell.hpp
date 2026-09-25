#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/toolbar.hpp"
#include "goreecloud/browser/unified_search_bar.hpp"
#include "goreecloud/browser/window_controller.hpp"

namespace goreecloud::browser {

enum class ChromeDensity {
  compact,
  comfortable,
  touch,
};

enum class ChromeSurfaceRole {
  canvas,
  raised,
  glaze,
  overlay,
};

struct ChromeTabPresentation {
  std::string id;
  std::string title;
  bool active{false};
  bool pinned{false};
  bool private_context{false};
  bool loading{false};
};

struct UnifiedSearchPresentation {
  std::string display_text;
  bool focused{false};
  bool secure_context{false};
  bool reader_available{false};
  bool bookmarked_local{false};
  bool bookmarked_goreecloud{false};
};

struct BrowserChromeState {
  std::vector<ChromeTabPresentation> tabs;
  UnifiedSearchPresentation unified_search;
  ChromeDensity density{ChromeDensity::comfortable};
  ChromeSurfaceRole toolbar_surface{ChromeSurfaceRole::glaze};
  bool reduced_motion{false};
  bool reduced_transparency{false};
  bool increased_contrast{false};
};

inline std::string browser_tab_title(std::string_view url, std::string_view engine_title) {
  if (url == kNewTabUrl) return "New Tab";
  if (url == kHomeUrl) return "Home";
  if (url == kSettingsUrl) return "Settings";
  if (url == kPrivateStartUrl) return "Private Browsing";
  if (!engine_title.empty()) return std::string{engine_title};
  return std::string{url};
}

inline std::string browser_location_text(std::string_view url) {
  if (is_goreecloud_internal_url(url)) return {};
  return std::string{url};
}

class BrowserChromeShell {
 public:
  explicit BrowserChromeShell(WindowController& window) : window_(window) {}

  [[nodiscard]] BrowserChromeState snapshot() const {
    BrowserChromeState state;
    const auto* active = window_.active_tab();
    if (active) {
      const auto navigation = active->engine_view().navigation_state();
      state.unified_search.display_text = browser_location_text(navigation.url);
    }

    for (const auto* tab : window_.tab_views()) {
      const auto navigation = tab->engine_view().navigation_state();
      state.tabs.push_back(ChromeTabPresentation{
          .id = tab->id(),
          .title = browser_tab_title(navigation.url, navigation.title),
          .active = active && active->id() == tab->id(),
          .pinned = false,
          .private_context = window_.private_window(),
          .loading = navigation.loading,
      });
    }
    return state;
  }

  void invoke(ToolbarItem item) {
    switch (item) {
      case ToolbarItem::back:
        window_.back();
        break;
      case ToolbarItem::forward:
        window_.forward();
        break;
      case ToolbarItem::refresh:
        window_.reload_or_stop();
        break;
      case ToolbarItem::home:
        window_.open_home();
        break;
      case ToolbarItem::settings:
        window_.open_settings();
        break;
      case ToolbarItem::unified_search:
      case ToolbarItem::advanced_download_manager:
      case ToolbarItem::privacy_shield:
      case ToolbarItem::wardveil_security:
      case ToolbarItem::clipboard:
      case ToolbarItem::dns_cache:
      case ToolbarItem::advanced_proxy_manager:
        // These open Browser-owned panels/controllers. Wiring is intentionally
        // separate from engine-owned navigation.
        break;
    }
  }

  [[nodiscard]] static constexpr auto toolbar_items() {
    return kDefaultToolbar;
  }

  [[nodiscard]] static constexpr auto unified_search_controls() {
    return kUnifiedSearchBarControls;
  }

 private:
  WindowController& window_;
};

inline constexpr bool kChromeOwnedByGoreeCloud = true;
inline constexpr bool kInheritedChromiumChromeAllowed = false;
inline constexpr bool kLatestApprovedGlazeRequiredForChrome = true;
inline constexpr std::uint16_t kMinimumInteractiveTargetPx = 44;
inline constexpr std::uint16_t kComfortableInteractiveTargetPx = 48;

static_assert(kChromeOwnedByGoreeCloud);
static_assert(!kInheritedChromiumChromeAllowed);
static_assert(kLatestApprovedGlazeRequiredForChrome);
static_assert(kDefaultToolbar.size() == 12);
static_assert(kUnifiedSearchBarControls.size() == 3);

}  // namespace goreecloud::browser
