#include <cassert>
#include <string_view>
#include <vector>

#include "goreecloud/browser/health.hpp"

using namespace goreecloud::browser;

namespace {

BrowserHealthInput healthy_core() {
  return BrowserHealthInput{
      .process_loop_healthy = true,
      .render_engine_available = true,
      .local_state_available = true,
      .browser_ui_available = true,
      .dependencies = {},
  };
}

HealthSignal signal(
    HealthSubsystem subsystem,
    HealthCondition condition,
    HealthFreshness freshness = HealthFreshness::current,
    bool required = false) {
  return HealthSignal{
      .subsystem = subsystem,
      .condition = condition,
      .freshness = freshness,
      .required_for_core_readiness = required,
  };
}

}  // namespace

int main() {
  static_assert(kBrowserHealthContractVersion == 1);
  static_assert(kBrowserHealthContractId ==
                std::string_view{"goreecloud.browser.health.v1"});

  {
    const auto snapshot = evaluate_browser_health(healthy_core());
    assert(snapshot.input_valid);
    assert(snapshot.liveness == HealthCondition::healthy);
    assert(snapshot.readiness == ReadinessCondition::ready);
    assert(snapshot.primary_reason == HealthReason::none);
  }

  {
    auto input = healthy_core();
    input.process_loop_healthy = false;
    const auto snapshot = evaluate_browser_health(input);
    assert(snapshot.liveness == HealthCondition::unavailable);
    assert(snapshot.readiness == ReadinessCondition::blocked);
    assert(snapshot.primary_reason == HealthReason::process_loop_unhealthy);
  }

  {
    auto input = healthy_core();
    input.render_engine_available = false;
    const auto snapshot = evaluate_browser_health(input);
    assert(snapshot.liveness == HealthCondition::healthy);
    assert(snapshot.readiness == ReadinessCondition::blocked);
    assert(snapshot.primary_reason == HealthReason::render_engine_unavailable);
  }

  {
    auto input = healthy_core();
    input.dependencies = {
        signal(HealthSubsystem::search, HealthCondition::unavailable),
    };
    const auto snapshot = evaluate_browser_health(input);
    assert(snapshot.liveness == HealthCondition::healthy);
    assert(snapshot.readiness == ReadinessCondition::degraded);
    assert(snapshot.primary_reason == HealthReason::dependency_unavailable);
  }

  {
    auto input = healthy_core();
    input.dependencies = {
        signal(HealthSubsystem::policy, HealthCondition::unavailable,
               HealthFreshness::current, true),
    };
    const auto snapshot = evaluate_browser_health(input);
    assert(snapshot.readiness == ReadinessCondition::blocked);
    assert(snapshot.primary_reason == HealthReason::dependency_unavailable);
  }

  {
    auto input = healthy_core();
    input.dependencies = {
        signal(HealthSubsystem::privacy_shield, HealthCondition::healthy,
               HealthFreshness::stale, true),
    };
    const auto snapshot = evaluate_browser_health(input);
    assert(snapshot.readiness == ReadinessCondition::blocked);
    assert(snapshot.primary_reason == HealthReason::evidence_stale);
  }

  {
    auto input = healthy_core();
    input.dependencies = {
        signal(HealthSubsystem::observability, HealthCondition::unknown,
               HealthFreshness::unknown, false),
    };
    const auto snapshot = evaluate_browser_health(input);
    assert(snapshot.readiness == ReadinessCondition::degraded);
    assert(snapshot.primary_reason == HealthReason::evidence_freshness_unknown);
  }

  {
    auto input = healthy_core();
    input.dependencies = {
        signal(HealthSubsystem::search, HealthCondition::healthy),
        signal(HealthSubsystem::search, HealthCondition::healthy),
    };
    const auto snapshot = evaluate_browser_health(input);
    assert(!snapshot.input_valid);
    assert(snapshot.readiness == ReadinessCondition::blocked);
    assert(snapshot.primary_reason == HealthReason::duplicate_subsystem_signal);
  }

  assert(health_condition_name(HealthCondition::healthy) == "healthy");
  assert(readiness_condition_name(ReadinessCondition::blocked) == "blocked");
  assert(health_reason_name(HealthReason::dependency_unknown) ==
         "dependency-unknown");

  return 0;
}
