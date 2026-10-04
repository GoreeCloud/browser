#pragma once

#include <cstdint>
#include <string_view>

namespace goreecloud::browser {

inline constexpr std::string_view kGlazeUiPolicy = "latest-approved-stable";
inline constexpr std::string_view kCurrentGlazeUiStableVersion = "1.7.0";
inline constexpr std::string_view kCurrentGlazeUiStableRevision =
    "1a5756daed2294155be2e9972b24f580f6222b7b";
inline constexpr std::string_view kCurrentGlazeUiReviewedImplementationAnchor =
    "a7180679ea851389e0f3004515f9a25f420e716d";
inline constexpr std::string_view kCurrentGlazeUiQualificationSourceAnchor =
    "7c4ded83d7a8725165bb6a55dfb175667cc9589e";
inline constexpr std::string_view kCurrentGlazeUiQualificationIntegrationRevision =
    "1a5756daed2294155be2e9972b24f580f6222b7b";
inline constexpr std::string_view kGlazeUiOpticalBaselineVersion = "1.4.1";
inline constexpr std::string_view kGlazeUiOpticalBaselineRevision =
    "4fab9da0fad2e5c974e0e66ec88632c61745751c";
inline constexpr std::string_view kGlazeUiImmediateRollbackVersion = "1.6.0";

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

  // V1.7 source mapping inherits the accepted V1.6 Stable runtime and
  // authority boundaries while Browser-local V1.7 acceptance remains open.
  // These capabilities describe presentation only; none create authority.
  bool semantic_context_normalization{true};
  bool capability_state_with_provenance{true};
  bool provider_conflict_fail_closed{true};
  bool accessibility_precedence{true};
  bool runtime_pressure_cost_reduction{true};
  bool capability_aware_controls{true};
  bool explicit_unavailable_explanations{true};
  bool privacy_safe_diagnostics{true};

  // V1.7 Browser-adoption invariants inherit V1.6 runtime behavior. These describe presentation behavior
  // only and do not grant Browser or Glaze any provider-owned authority.
  bool complete_component_states{true};
  bool large_text_reflow{true};
  bool non_color_semantic_indicators{true};
  bool distinguish_capability_states{true};
  bool responsive_task_continuity{true};
  bool localization_reflow{true};
  bool truthful_status_provenance{true};
  bool bounded_visual_complexity{true};
  bool power_aware_effect_reduction{true};

  // Inherited authority invariants remain mandatory under the V1.7 mapping.
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
static_assert(kBrowserGlazeCapabilities.complete_component_states);
static_assert(kBrowserGlazeCapabilities.large_text_reflow);
static_assert(kBrowserGlazeCapabilities.non_color_semantic_indicators);
static_assert(kBrowserGlazeCapabilities.distinguish_capability_states);
static_assert(kBrowserGlazeCapabilities.responsive_task_continuity);
static_assert(kBrowserGlazeCapabilities.localization_reflow);
static_assert(kBrowserGlazeCapabilities.truthful_status_provenance);
static_assert(kBrowserGlazeCapabilities.bounded_visual_complexity);
static_assert(kBrowserGlazeCapabilities.power_aware_effect_reduction);
static_assert(!kBrowserGlazeCapabilities.infer_authorization);
static_assert(!kBrowserGlazeCapabilities.infer_provider_precedence);
static_assert(!kBrowserGlazeCapabilities.automatic_consequential_execution);
static_assert(!kBrowserGlazeCapabilities.automatic_fallback_execution);

// Browser-owned user-facing surfaces target the current consumer-eligible
// Glaze V1.7 / 1.7.0 release. Canonical GoreeCloud lifecycle is Anchor;
// retained "Stable" naming is compatibility terminology for the consumer
// release channel. Exact source and qualification revisions are pinned so CI
// and acceptance evidence cannot silently drift. Browser-local acceptance
// remains independent.

}  // namespace goreecloud::browser
