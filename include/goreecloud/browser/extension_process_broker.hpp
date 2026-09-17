#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/extension_runtime_authority.hpp"

namespace goreecloud::browser {

inline constexpr std::uint32_t kExtensionProcessIsolationProfileVersion = 1;

// Browser-owned minimum isolation contract that a platform-specific launcher
// must satisfy before an extension process can acquire runtime authority.
// This contract does not itself create an operating-system sandbox.
struct ExtensionProcessIsolationProfile {
  std::uint32_t version{kExtensionProcessIsolationProfileVersion};
  bool dedicated_process_required{true};
  bool broker_channel_required{true};
  bool browser_internal_access_blocked{true};
  bool other_extension_access_blocked{true};
  bool goreecloud_service_access_blocked{true};
  bool ambient_operating_system_authority_blocked{true};
};

inline constexpr ExtensionProcessIsolationProfile
extension_process_isolation_profile() noexcept {
  return {};
}

struct ExtensionProcessLaunchRequest {
  ExtensionRuntimeIdentity identity;
  std::vector<std::string> entry_points;
  ExtensionTrustState package_trust_state{ExtensionTrustState::unsigned_package};
  ExtensionProcessIsolationProfile isolation;
};

// A trusted Browser platform adapter returns this receipt only after launch.
// The receipt is evidence for Browser-side gating, not a production sandbox
// attestation or a substitute for platform-specific runtime validation.
struct ExtensionProcessLaunchReceipt {
  std::string runtime_instance_id;
  std::string platform_process_id;
  std::uint32_t isolation_profile_version{0};
  bool dedicated_process{false};
  bool broker_channel_established{false};
  bool browser_internal_access_blocked{false};
  bool other_extension_access_blocked{false};
  bool goreecloud_service_access_blocked{false};
  bool ambient_operating_system_authority_blocked{false};
};

inline bool valid_extension_process_launch_receipt(
    const ExtensionProcessLaunchRequest& request,
    const ExtensionProcessLaunchReceipt& receipt) {
  return request.isolation.version == kExtensionProcessIsolationProfileVersion &&
         request.isolation.dedicated_process_required &&
         request.isolation.broker_channel_required &&
         request.isolation.browser_internal_access_blocked &&
         request.isolation.other_extension_access_blocked &&
         request.isolation.goreecloud_service_access_blocked &&
         request.isolation.ambient_operating_system_authority_blocked &&
         receipt.runtime_instance_id == request.identity.runtime_instance_id &&
         valid_extension_runtime_identifier(receipt.runtime_instance_id) &&
         valid_extension_runtime_identifier(receipt.platform_process_id) &&
         receipt.isolation_profile_version == request.isolation.version &&
         receipt.dedicated_process && receipt.broker_channel_established &&
         receipt.browser_internal_access_blocked &&
         receipt.other_extension_access_blocked &&
         receipt.goreecloud_service_access_blocked &&
         receipt.ambient_operating_system_authority_blocked;
}

class ExtensionPlatformProcessLauncher {
 public:
  virtual ~ExtensionPlatformProcessLauncher() = default;

  [[nodiscard]] virtual std::optional<ExtensionProcessLaunchReceipt> launch(
      const ExtensionProcessLaunchRequest& request) = 0;

  // Returns true only when the platform adapter confirms that the addressed
  // process no longer executes extension work.
  [[nodiscard]] virtual bool terminate(
      std::string_view platform_process_id) = 0;
};

struct ExtensionBrokerProcess {
  std::string runtime_instance_id;
  std::string platform_process_id;
  bool platform_termination_confirmed{false};
};

class ExtensionProcessBroker {
 public:
  [[nodiscard]] bool launch(
      ExtensionPlatformProcessLauncher& launcher,
      const ExtensionManifest& manifest,
      ExtensionTrustState package_trust_state,
      ExtensionRuntimeIdentity identity,
      const ExtensionRuntimeLaunchDecision& decision) {
    // Trust metadata, package parsing, and installation state are intentionally
    // insufficient to spawn a process. Browser launch authority is explicit.
    if (!decision.browser_launch_authorized ||
        (identity.private_browsing && !decision.private_browsing_authorized) ||
        !validate_extension_manifest(manifest).accepted() ||
        manifest.id != identity.extension_id || manifest.entry_points.empty() ||
        !valid_extension_runtime_identifier(identity.extension_id) ||
        !valid_extension_runtime_identifier(identity.profile_id) ||
        !valid_extension_runtime_identifier(identity.runtime_instance_id) ||
        find_process(identity.runtime_instance_id) != processes_.end() ||
        authority_.session(identity.runtime_instance_id) != nullptr) {
      return false;
    }

    const auto authority_policy = extension_runtime_process_policy();
    if (!authority_policy.dedicated_process_required ||
        !authority_policy.broker_only_privileged_api ||
        authority_policy.direct_browser_internal_access ||
        authority_policy.direct_other_extension_access ||
        authority_policy.direct_goreecloud_service_access ||
        authority_policy.ambient_operating_system_authority) {
      return false;
    }

    ExtensionProcessLaunchRequest request{
        .identity = identity,
        .entry_points = manifest.entry_points,
        .package_trust_state = package_trust_state,
        .isolation = extension_process_isolation_profile(),
    };

    auto receipt = launcher.launch(request);
    if (!receipt.has_value()) {
      return false;
    }

    if (!valid_extension_process_launch_receipt(request, *receipt) ||
        find_platform_process(receipt->platform_process_id) != processes_.end()) {
      rollback_unregistered_process(launcher, *receipt);
      return false;
    }

    if (!authority_.register_runtime(manifest, package_trust_state,
                                     std::move(identity), decision)) {
      rollback_unregistered_process(launcher, *receipt);
      return false;
    }

    processes_.push_back(ExtensionBrokerProcess{
        .runtime_instance_id = receipt->runtime_instance_id,
        .platform_process_id = receipt->platform_process_id,
    });
    return true;
  }

  [[nodiscard]] std::optional<ExtensionRuntimeCapabilityToken>
  issue_capability(ExtensionPermissionLedger& permission_ledger,
                   const ExtensionRuntimeCapabilityRequest& request) {
    const auto process = find_process(request.runtime_instance_id);
    if (process == processes_.end() || process->platform_termination_confirmed) {
      return std::nullopt;
    }
    return authority_.issue_capability(permission_ledger, request);
  }

  [[nodiscard]] bool authorize_and_consume(
      std::uint64_t token_id,
      const ExtensionRuntimeDispatchContext& context) {
    const auto process = find_process(context.runtime_instance_id);
    if (process == processes_.end() || process->platform_termination_confirmed) {
      return false;
    }
    return authority_.authorize_and_consume(token_id, context);
  }

  [[nodiscard]] bool terminate(ExtensionPlatformProcessLauncher& launcher,
                               std::string_view runtime_instance_id) {
    auto process = find_process(runtime_instance_id);
    if (process == processes_.end()) {
      return false;
    }
    if (process->platform_termination_confirmed) {
      return true;
    }

    // Revoke Browser authority before asking the platform to terminate. Even
    // if platform termination must be retried, the process has zero privileged
    // Browser API authority after this point.
    const auto* session = authority_.session(runtime_instance_id);
    if (session != nullptr && session->state != ExtensionRuntimeState::terminated) {
      if (!authority_.terminate(runtime_instance_id)) {
        return false;
      }
    }

    if (!launcher.terminate(process->platform_process_id)) {
      return false;
    }
    process->platform_termination_confirmed = true;
    return true;
  }

  [[nodiscard]] const ExtensionRuntimeSession* session(
      std::string_view runtime_instance_id) const noexcept {
    return authority_.session(runtime_instance_id);
  }

  [[nodiscard]] const ExtensionBrokerProcess* process(
      std::string_view runtime_instance_id) const noexcept {
    const auto found = std::find_if(
        processes_.begin(), processes_.end(),
        [runtime_instance_id](const ExtensionBrokerProcess& candidate) {
          return candidate.runtime_instance_id == runtime_instance_id;
        });
    return found == processes_.end() ? nullptr : &*found;
  }

 private:
  std::vector<ExtensionBrokerProcess>::iterator find_process(
      std::string_view runtime_instance_id) {
    return std::find_if(
        processes_.begin(), processes_.end(),
        [runtime_instance_id](const ExtensionBrokerProcess& candidate) {
          return candidate.runtime_instance_id == runtime_instance_id;
        });
  }

  std::vector<ExtensionBrokerProcess>::iterator find_platform_process(
      std::string_view platform_process_id) {
    return std::find_if(
        processes_.begin(), processes_.end(),
        [platform_process_id](const ExtensionBrokerProcess& candidate) {
          return candidate.platform_process_id == platform_process_id;
        });
  }

  static void rollback_unregistered_process(
      ExtensionPlatformProcessLauncher& launcher,
      const ExtensionProcessLaunchReceipt& receipt) {
    if (valid_extension_runtime_identifier(receipt.platform_process_id)) {
      static_cast<void>(launcher.terminate(receipt.platform_process_id));
    }
  }

  ExtensionRuntimeAuthority authority_;
  std::vector<ExtensionBrokerProcess> processes_;
};

}  // namespace goreecloud::browser
