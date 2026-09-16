#pragma once

#include <cstdint>
#include <string_view>

namespace goreecloud::browser {

inline constexpr std::string_view kGlazeUiPolicy = "latest-approved-stable";
inline constexpr std::string_view kCurrentGlazeUiStableVersion = "1.5.0";
inline constexpr std::string_view kCurrentGlazeUiStableRevision =
    "b7fa8164bfdeaa1dc0acb21b770e7601120da04e";
inline constexpr std::string_view kCurrentGlazeUiReviewedImplementationAnchor =
    "ee1032a0822ab8e103f8afe48e5c1859fde65cc9";
inline constexpr std::string_view kGlazeUiOpticalBaselineVersion = "1.4.1";
inline constexpr std::string_view kGlazeUiOpticalBaselineRevision =
    "4fab9da0fad2e5c974e0e66ec88632c61745751c";
inline constexpr std::string_view kGlazeUiImmediateRollbackVersion = "1.4.1";

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

  // V1.5 Stable presentation-resolution capabilities. These describe what the
  // Browser presentation layer may consume or express; none create authority.
  bool semantic_context_normalization{true};
  bool capability_state_with_provenance{true};
  bool provider_conflict_fail_closed{true};
  bool accessibility_precedence{true};
  bool runtime_pressure_cost_reduction{true};
  bool capability_aware_controls{true};
  bool explicit_unavailable_explanations{true};
  bool privacy_safe_diagnostics{true};

  // V1.5 authority invariants.
  bool infer_authorization{false};
  bool infer_provider_precedence{false};
  bool automatic_consequential_execution{false};
  bool automatic_fallback_execution{false};
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
static_assert(kBrowserGlazeCapabilities.semantic_context_normalization);
static_assert(kBrowserGlazeCapabilities.capability_state_with_provenance);
static_assert(kBrowserGlazeCapabilities.provider_conflict_fail_closed);
static_assert(kBrowserGlazeCapabilities.accessibility_precedence);
static_assert(kBrowserGlazeCapabilities.runtime_pressure_cost_reduction);
static_assert(kBrowserGlazeCapabilities.capability_aware_controls);
static_assert(kBrowserGlazeCapabilities.explicit_unavailable_explanations);
static_assert(kBrowserGlazeCapabilities.privacy_safe_diagnostics);
static_assert(!kBrowserGlazeCapabilities.infer_authorization);
static_assert(!kBrowserGlazeCapabilities.infer_provider_precedence);
static_assert(!kBrowserGlazeCapabilities.automatic_consequential_execution);
static_assert(!kBrowserGlazeCapabilities.automatic_fallback_execution);

// Browser-owned user-facing surfaces track the current approved Stable Glaze UI
// release. Version and authority revisions are intentionally pinned so CI and
// acceptance evidence cannot silently drift to a different design-system
// baseline. Consumer acceptance remains Browser-local and independent.

}  // namespace goreecloud::browser
