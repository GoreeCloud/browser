#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace goreecloud::browser {

inline constexpr std::uint32_t kBrowserHealthContractVersion = 1;
inline constexpr std::string_view kBrowserHealthContractId =
    "goreecloud.browser.health.v1";

enum class HealthCondition {
  healthy,
  degraded,
  unavailable,
  unknown,
};

enum class ReadinessCondition {
  ready,
  degraded,
  blocked,
};

enum class HealthFreshness {
  current,
  stale,
  unknown,
};

enum class HealthSubsystem {
  browser_core,
  render_engine,
  local_state,
  browser_ui,
  network_path,
  search,
  privacy_shield,
  wardveil_security,
  everkeep,
  mesh,
  identity,
  policy,
  observability,
  sync,
};

enum class HealthReason {
  none,
  process_loop_unhealthy,
  render_engine_unavailable,
  local_state_unavailable,
  browser_ui_unavailable,
  dependency_degraded,
  dependency_unavailable,
  dependency_unknown,
  evidence_stale,
  evidence_freshness_unknown,
  duplicate_subsystem_signal,
};

struct HealthSignal {
  HealthSubsystem subsystem{HealthSubsystem::browser_core};
  HealthCondition condition{HealthCondition::unknown};
  HealthFreshness freshness{HealthFreshness::unknown};
  bool required_for_core_readiness{false};
};

struct BrowserHealthInput {
  bool process_loop_healthy{false};
  bool render_engine_available{false};
  bool local_state_available{false};
  bool browser_ui_available{false};
  std::vector<HealthSignal> dependencies;
};

struct BrowserHealthSnapshot {
  std::uint32_t contract_version{kBrowserHealthContractVersion};
  HealthCondition liveness{HealthCondition::unavailable};
  ReadinessCondition readiness{ReadinessCondition::blocked};
  HealthReason primary_reason{HealthReason::none};
  bool input_valid{true};
  std::vector<HealthSignal> dependencies;
};

inline constexpr std::string_view health_condition_name(
    HealthCondition condition) {
  switch (condition) {
    case HealthCondition::healthy:
      return "healthy";
    case HealthCondition::degraded:
      return "degraded";
    case HealthCondition::unavailable:
      return "unavailable";
    case HealthCondition::unknown:
      return "unknown";
  }
  return "unknown";
}

inline constexpr std::string_view readiness_condition_name(
    ReadinessCondition condition) {
  switch (condition) {
    case ReadinessCondition::ready:
      return "ready";
    case ReadinessCondition::degraded:
      return "degraded";
    case ReadinessCondition::blocked:
      return "blocked";
  }
  return "blocked";
}

inline constexpr std::string_view health_reason_name(HealthReason reason) {
  switch (reason) {
    case HealthReason::none:
      return "none";
    case HealthReason::process_loop_unhealthy:
      return "process-loop-unhealthy";
    case HealthReason::render_engine_unavailable:
      return "render-engine-unavailable";
    case HealthReason::local_state_unavailable:
      return "local-state-unavailable";
    case HealthReason::browser_ui_unavailable:
      return "browser-ui-unavailable";
    case HealthReason::dependency_degraded:
      return "dependency-degraded";
    case HealthReason::dependency_unavailable:
      return "dependency-unavailable";
    case HealthReason::dependency_unknown:
      return "dependency-unknown";
    case HealthReason::evidence_stale:
      return "evidence-stale";
    case HealthReason::evidence_freshness_unknown:
      return "evidence-freshness-unknown";
    case HealthReason::duplicate_subsystem_signal:
      return "duplicate-subsystem-signal";
  }
  return "dependency-unknown";
}

inline bool health_dependency_signals_unique(
    const std::vector<HealthSignal>& dependencies) {
  std::unordered_set<int> seen;
  for (const auto& signal : dependencies) {
    if (!seen.insert(static_cast<int>(signal.subsystem)).second) {
      return false;
    }
  }
  return true;
}

inline BrowserHealthSnapshot evaluate_browser_health(
    const BrowserHealthInput& input) {
  BrowserHealthSnapshot snapshot{
      .contract_version = kBrowserHealthContractVersion,
      .liveness = input.process_loop_healthy ? HealthCondition::healthy
                                             : HealthCondition::unavailable,
      .readiness = ReadinessCondition::ready,
      .primary_reason = HealthReason::none,
      .input_valid = true,
      .dependencies = input.dependencies,
  };

  if (!health_dependency_signals_unique(input.dependencies)) {
    snapshot.input_valid = false;
    snapshot.readiness = ReadinessCondition::blocked;
    snapshot.primary_reason = HealthReason::duplicate_subsystem_signal;
    return snapshot;
  }

  if (!input.process_loop_healthy) {
    snapshot.readiness = ReadinessCondition::blocked;
    snapshot.primary_reason = HealthReason::process_loop_unhealthy;
    return snapshot;
  }
  if (!input.render_engine_available) {
    snapshot.readiness = ReadinessCondition::blocked;
    snapshot.primary_reason = HealthReason::render_engine_unavailable;
    return snapshot;
  }
  if (!input.local_state_available) {
    snapshot.readiness = ReadinessCondition::blocked;
    snapshot.primary_reason = HealthReason::local_state_unavailable;
    return snapshot;
  }
  if (!input.browser_ui_available) {
    snapshot.readiness = ReadinessCondition::blocked;
    snapshot.primary_reason = HealthReason::browser_ui_unavailable;
    return snapshot;
  }

  for (const auto& signal : input.dependencies) {
    if (signal.required_for_core_readiness) {
      if (signal.condition == HealthCondition::unavailable) {
        snapshot.readiness = ReadinessCondition::blocked;
        snapshot.primary_reason = HealthReason::dependency_unavailable;
        return snapshot;
      }
      if (signal.condition == HealthCondition::unknown) {
        snapshot.readiness = ReadinessCondition::blocked;
        snapshot.primary_reason = HealthReason::dependency_unknown;
        return snapshot;
      }
      if (signal.freshness == HealthFreshness::stale) {
        snapshot.readiness = ReadinessCondition::blocked;
        snapshot.primary_reason = HealthReason::evidence_stale;
        return snapshot;
      }
      if (signal.freshness == HealthFreshness::unknown) {
        snapshot.readiness = ReadinessCondition::blocked;
        snapshot.primary_reason = HealthReason::evidence_freshness_unknown;
        return snapshot;
      }
      if (signal.condition == HealthCondition::degraded) {
        snapshot.readiness = ReadinessCondition::degraded;
        if (snapshot.primary_reason == HealthReason::none) {
          snapshot.primary_reason = HealthReason::dependency_degraded;
        }
      }
      continue;
    }

    if (signal.condition != HealthCondition::healthy ||
        signal.freshness != HealthFreshness::current) {
      snapshot.readiness = ReadinessCondition::degraded;
      if (snapshot.primary_reason == HealthReason::none) {
        if (signal.freshness == HealthFreshness::stale) {
          snapshot.primary_reason = HealthReason::evidence_stale;
        } else if (signal.freshness == HealthFreshness::unknown) {
          snapshot.primary_reason = HealthReason::evidence_freshness_unknown;
        } else if (signal.condition == HealthCondition::unavailable) {
          snapshot.primary_reason = HealthReason::dependency_unavailable;
        } else if (signal.condition == HealthCondition::unknown) {
          snapshot.primary_reason = HealthReason::dependency_unknown;
        } else {
          snapshot.primary_reason = HealthReason::dependency_degraded;
        }
      }
    }
  }

  return snapshot;
}

}  // namespace goreecloud::browser
