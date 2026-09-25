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
  component_states,
  large_text,
  status_provenance,
  performance_adaptation,
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
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::component_states,
                               "Applicable controls expose default, hover, focus, pressed, selected, disabled, loading, and error states without color-only meaning."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::large_text,
                               "Large-text and text-expansion behavior reflows rather than clipping or reducing supported interaction targets."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::status_provenance,
                               "Privacy, security, connectivity, source, and capability status presentation remains attributable to authoritative state and leaves unknown state unverified."},
    GlazeAcceptanceRequirement{GlazeAcceptanceArea::performance_adaptation,
                               "Presentation may reduce optional effects under runtime pressure while preserving semantics, interaction capability, and task continuity."},
};

inline constexpr bool kGlazeAcceptanceRequiredForProduction = true;

static_assert(kCurrentGlazeUiStableVersion == std::string_view{"1.6.0"});
static_assert(kCurrentGlazeUiStableRevision ==
              std::string_view{"a7180679ea851389e0f3004515f9a25f420e716d"});
static_assert(kCurrentGlazeUiQualificationSourceAnchor ==
              std::string_view{"c7509c79256b04b0aa67cb9dd0737d7588e0ae4a"});
static_assert(kCurrentGlazeUiQualificationIntegrationRevision ==
              std::string_view{"354f5759385c28596fcfec26a3ad525e89fb1c35"});
static_assert(kGlazeUiOpticalBaselineVersion == std::string_view{"1.4.1"});
static_assert(kGlazeUiImmediateRollbackVersion == std::string_view{"1.5.1"});
static_assert(kGlazeAcceptanceRequiredForProduction);

}  // namespace goreecloud::browser
