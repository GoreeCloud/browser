#pragma once

#include <array>
#include <string_view>

#include "goreecloud/browser/glaze.hpp"

namespace goreecloud::browser {

enum class GlazeAcceptanceArea {
  semantic_color,
  material_hierarchy,
  connected_transformation,
  live_surfaces,
  navigation_capsule,
  appearance_clarity_expression,
  iconography,
  motion,
  accessibility,
  responsive_layout,
  safe_areas,
  density,
  mixed_input,
  localization,
  security_truth,
  privacy_truth,
  capability_truth,
  authority_conflicts,
  explainable_adaptation,
};

struct GlazeAcceptanceRequirement {
  GlazeAcceptanceArea area;
  std::string_view evidence;
};

inline constexpr std::array kGlazeAcceptanceRequirements{
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::semantic_color,
                               "Browser surfaces use Glaze semantic roles rather than ad-hoc color meaning."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::material_hierarchy,
                               "Canvas, Surface, Soft Glaze, Glaze, Deep Glaze, and Live Glaze are used by purpose."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::connected_transformation,
                               "Connected transformations preserve task identity and remain interruptible."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::live_surfaces,
                               "Live surfaces are bounded to active context and degrade safely when motion or transparency is reduced."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::navigation_capsule,
                               "Navigation capsules preserve current destination, focus order, and minimum target size across layouts."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::appearance_clarity_expression,
                               "Appearance, clarity, and expression modes do not alter security, privacy, capability, or functional meaning."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::iconography,
                               "Browser-owned icons follow current Glaze iconography contracts."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::motion,
                               "Motion is purposeful, interruptible, and has reduced-motion substitutions."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::accessibility,
                               "Focus, contrast, transparency fallbacks, target floors, assistive semantics, and accessibility precedence are validated."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::responsive_layout,
                               "Browser chrome and owned surfaces adapt across supported Glaze layout classes."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::safe_areas,
                               "System insets, cutouts, gesture regions, hinges, and window chrome constraints are respected."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::density,
                               "Density changes preserve hierarchy, target floors, capability truth, and task usability."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::mixed_input,
                               "Touch, pointer, keyboard, and other supported input modes retain complete interaction states."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::localization,
                               "Layout tolerates localization, text expansion, directionality, and intrinsic overflow."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::security_truth,
                               "Glaze presents Wardveil evidence-backed state without inventing security truth."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::privacy_truth,
                               "Glaze presents Privacy Shield state without inventing privacy truth or permission."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::capability_truth,
                               "Capability-aware controls consume authoritative capability state and never infer authorization or automatic execution."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::authority_conflicts,
                               "Missing or conflicting provider ownership fails closed without inferred provider precedence."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::explainable_adaptation,
                               "Unavailable, degraded, recovery, and fallback presentation remains privacy-safe, user-initiated where consequential, and free of sensitive diagnostic payloads."},
};

inline constexpr bool kGlazeAcceptanceRequiredForProduction = true;

static_assert(kCurrentGlazeUiStableVersion == std::string_view{"1.5.0"});
static_assert(kCurrentGlazeUiStableRevision ==
              std::string_view{"b7fa8164bfdeaa1dc0acb21b770e7601120da04e"});
static_assert(kGlazeUiOpticalBaselineVersion == std::string_view{"1.4.1"});
static_assert(kGlazeAcceptanceRequiredForProduction);

}  // namespace goreecloud::browser
