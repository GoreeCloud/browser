#pragma once

#include <cstdint>
#include <string_view>

namespace goreecloud::browser {

inline constexpr std::string_view kGlazeUiPolicy = "latest-approved-stable";
inline constexpr std::string_view kCurrentGlazeUiStableVersion = "1.4.0";
inline constexpr std::string_view kCurrentGlazeUiStableRevision =
    "84cb3db4884042f0fa25ed6d475a127fb110f596";
inline constexpr std::string_view kGlazeUiHumanValidationFollowUp = "1.4.1";

enum class GlazeSurface {
  canvas,
  surface,
  soft_glaze,
  glaze,
  deep_glaze,
  live_glaze,
};

enum class LayoutClass { compact, medium, expanded, wide };
enum class AppearanceMode { system, light, dark, deep_dark };
enum class ClarityMode { clear, balanced, solid };
enum class ExpressionMode { calm, balanced, expressive };
enum class DensityMode { compact, standard, comfortable };

enum class InteractionState {
  idle,
  hover,
  pressed,
  selected,
  expanded,
  disabled,
  read_only,
  loading,
  invalid,
  success,
};

struct AccessibilityPreferences {
  bool reduced_motion{false};
  bool reduced_transparency{false};
  bool increased_contrast{false};
  bool forced_colors{false};
};

struct GlazeCapabilities {
  std::uint16_t minimum_target_px{48};
  std::uint16_t television_target_px{56};
  bool visible_keyboard_focus{true};
  bool solid_transparency_fallback{true};
  bool adaptive_layouts{true};
  bool semantic_color{true};
  bool governed_iconography{true};
  bool interruptible_motion{true};
  bool safe_area_support{true};
  bool mixed_input_modality{true};
  bool connected_transformation{true};
  bool live_surfaces{true};
  bool navigation_capsule{true};
  bool optical_intelligence{true};
};

/**
 * Browser-specific V1.4 trust boundary for Browser-owned chrome.
 *
 * Web/page content is an untrusted rendering plane. It cannot tint or otherwise
 * drive trusted chrome, and optical appearance cannot manufacture security or
 * privacy truth. Environmental color memory is therefore disabled for trusted
 * browser chrome even though generic Glaze V1.4 permits bounded decorative use.
 */
struct BrowserGlazeOpticalPolicy {
  bool page_content_may_drive_trusted_chrome{false};
  bool origin_identity_may_drive_trusted_chrome{false};
  bool favicon_color_may_drive_trusted_chrome{false};
  bool page_theme_color_may_drive_trusted_chrome{false};
  bool security_state_may_be_inferred_from_optics{false};
  bool privacy_state_may_be_inferred_from_optics{false};
  bool telemetry_required{false};
  bool remote_context_required{false};
  std::uint8_t trusted_chrome_memory_tint_percent{0};
};

struct GlazeContext {
  AppearanceMode appearance{AppearanceMode::system};
  ClarityMode clarity{ClarityMode::balanced};
  ExpressionMode expression{ExpressionMode::balanced};
  LayoutClass layout{LayoutClass::medium};
  DensityMode density{DensityMode::standard};
  AccessibilityPreferences accessibility;
};

inline constexpr GlazeCapabilities kBrowserGlazeCapabilities{};
inline constexpr BrowserGlazeOpticalPolicy kBrowserGlazeOpticalPolicy{};

static_assert(kBrowserGlazeCapabilities.minimum_target_px >= 48);
static_assert(kBrowserGlazeCapabilities.television_target_px >= 56);
static_assert(kBrowserGlazeCapabilities.visible_keyboard_focus);
static_assert(kBrowserGlazeCapabilities.solid_transparency_fallback);
static_assert(kBrowserGlazeCapabilities.semantic_color);
static_assert(kBrowserGlazeCapabilities.governed_iconography);
static_assert(kBrowserGlazeCapabilities.interruptible_motion);
static_assert(kBrowserGlazeCapabilities.connected_transformation);
static_assert(kBrowserGlazeCapabilities.live_surfaces);
static_assert(kBrowserGlazeCapabilities.navigation_capsule);
static_assert(kBrowserGlazeCapabilities.optical_intelligence);

static_assert(!kBrowserGlazeOpticalPolicy.page_content_may_drive_trusted_chrome);
static_assert(!kBrowserGlazeOpticalPolicy.origin_identity_may_drive_trusted_chrome);
static_assert(!kBrowserGlazeOpticalPolicy.favicon_color_may_drive_trusted_chrome);
static_assert(!kBrowserGlazeOpticalPolicy.page_theme_color_may_drive_trusted_chrome);
static_assert(!kBrowserGlazeOpticalPolicy.security_state_may_be_inferred_from_optics);
static_assert(!kBrowserGlazeOpticalPolicy.privacy_state_may_be_inferred_from_optics);
static_assert(!kBrowserGlazeOpticalPolicy.telemetry_required);
static_assert(!kBrowserGlazeOpticalPolicy.remote_context_required);
static_assert(kBrowserGlazeOpticalPolicy.trusted_chrome_memory_tint_percent == 0);

// Browser-owned user-facing surfaces track the current approved Stable Glaze UI
// release. Version and revision are intentionally pinned so CI and acceptance
// evidence cannot silently drift to a different design-system baseline.

}  // namespace goreecloud::browser
