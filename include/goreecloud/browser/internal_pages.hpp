#pragma once

#include <string_view>

namespace goreecloud::browser {

enum class InternalPage {
  new_tab,
  home,
  settings,
  private_start,
  renderer_failure,
};

inline constexpr std::string_view kNewTabUrl{"goreecloud://new-tab"};
inline constexpr std::string_view kHomeUrl{"goreecloud://home"};
inline constexpr std::string_view kSettingsUrl{"goreecloud://settings"};
inline constexpr std::string_view kPrivateStartUrl{"goreecloud://private"};
inline constexpr std::string_view kRendererFailureUrl{"goreecloud://renderer-failure"};

[[nodiscard]] constexpr std::string_view internal_page_url(InternalPage page) {
  switch (page) {
    case InternalPage::new_tab: return kNewTabUrl;
    case InternalPage::home: return kHomeUrl;
    case InternalPage::settings: return kSettingsUrl;
    case InternalPage::private_start: return kPrivateStartUrl;
    case InternalPage::renderer_failure: return kRendererFailureUrl;
  }
  return "goreecloud://invalid";
}

[[nodiscard]] constexpr bool is_goreecloud_internal_url(std::string_view url) {
  return url.rfind("goreecloud://", 0) == 0;
}

inline constexpr bool kInheritedNewTabDisabled = true;
inline constexpr bool kInheritedHomeDisabled = true;
inline constexpr bool kInheritedSettingsDisabled = true;
inline constexpr bool kGlazeUiRequiredForOwnedInternalPages = true;

static_assert(kInheritedNewTabDisabled);
static_assert(kInheritedHomeDisabled);
static_assert(kInheritedSettingsDisabled);
static_assert(kGlazeUiRequiredForOwnedInternalPages);

}  // namespace goreecloud::browser
