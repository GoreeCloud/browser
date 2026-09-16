#include <cstdint>
#include <iostream>
#include <string_view>

#include "goreecloud/browser/extension_runtime_authority.hpp"

namespace {

bool require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "extension runtime authority smoke failed: " << message << '\n';
  }
  return condition;
}

goreecloud::browser::ExtensionManifest make_manifest(std::string_view id) {
  goreecloud::browser::ExtensionManifest manifest;
  manifest.id = std::string(id);
  manifest.name = "Runtime authority smoke";
  manifest.version = "1.0.0";
  manifest.entry_points = {"background/main.js"};
  manifest.requested_permissions = {"tabs.read", "page.read"};
  manifest.allowed_websites = {"https://example.test"};
  return manifest;
}

goreecloud::browser::ExtensionPermissionLease make_always_lease(
    std::uint64_t id,
    goreecloud::browser::ExtensionPermission permission,
    std::string_view profile_id,
    bool private_browsing_allowed) {
  goreecloud::browser::ExtensionPermissionLease lease;
  lease.id = id;
  lease.grant.permission = permission;
  lease.grant.profile_id = std::string(profile_id);
  lease.grant.scope = goreecloud::browser::ExtensionGrantScope::extension;
  lease.grant.lifetime = goreecloud::browser::ExtensionGrantLifetime::always;
  lease.grant.private_browsing_allowed = private_browsing_allowed;
  lease.grant.active = true;
  return lease;
}

goreecloud::browser::ExtensionRuntimeCapabilityRequest make_request(
    std::string_view extension_id,
    std::string_view profile_id,
    std::string_view runtime_instance_id,
    std::uint64_t now_millis,
    bool private_browsing = false) {
  goreecloud::browser::ExtensionRuntimeCapabilityRequest request;
  request.extension_id = extension_id;
  request.profile_id = profile_id;
  request.runtime_instance_id = runtime_instance_id;
  request.permission = goreecloud::browser::ExtensionPermission::read_tabs;
  request.private_browsing = private_browsing;
  request.browser_active = true;
  request.now_millis = now_millis;
  return request;
}

goreecloud::browser::ExtensionRuntimeDispatchContext make_dispatch(
    const goreecloud::browser::ExtensionRuntimeCapabilityRequest& request,
    std::uint64_t now_millis) {
  goreecloud::browser::ExtensionRuntimeDispatchContext context;
  context.extension_id = request.extension_id;
  context.profile_id = request.profile_id;
  context.runtime_instance_id = request.runtime_instance_id;
  context.permission = request.permission;
  context.website = request.website;
  context.tab_id = request.tab_id;
  context.browser_session_id = request.browser_session_id;
  context.private_browsing = request.private_browsing;
  context.tab_active = request.tab_active;
  context.website_active = request.website_active;
  context.browser_active = request.browser_active;
  context.now_millis = now_millis;
  return context;
}

}  // namespace

int main() {
  using namespace goreecloud::browser;

  const auto process_policy = extension_runtime_process_policy();
  if (!require(process_policy.dedicated_process_required,
               "dedicated process must be required") ||
      !require(process_policy.broker_only_privileged_api,
               "privileged API access must be broker-only") ||
      !require(!process_policy.direct_browser_internal_access,
               "direct Browser-internal access must be denied") ||
      !require(!process_policy.direct_other_extension_access,
               "direct cross-extension access must be denied") ||
      !require(!process_policy.direct_goreecloud_service_access,
               "direct GoreeCloud-service access must be denied") ||
      !require(!process_policy.ambient_operating_system_authority,
               "ambient OS authority must be denied")) {
    return 1;
  }

  const auto manifest = make_manifest("org.goreecloud.runtime-smoke");
  ExtensionRuntimeAuthority authority;

  // A valid package signature is trust metadata, not launch authority.
  ExtensionRuntimeIdentity signed_identity{
      .extension_id = manifest.id,
      .profile_id = "profile-personal",
      .runtime_instance_id = "runtime-signed-denied",
  };
  if (!require(!authority.register_runtime(
                   manifest, ExtensionTrustState::signed_package,
                   signed_identity, ExtensionRuntimeLaunchDecision{}),
               "signed package must not self-authorize execution")) {
    return 1;
  }

  ExtensionRuntimeIdentity identity{
      .extension_id = manifest.id,
      .profile_id = "profile-personal",
      .runtime_instance_id = "runtime-1",
  };
  ExtensionRuntimeLaunchDecision launch{
      .browser_launch_authorized = true,
  };
  if (!require(authority.register_runtime(
                   manifest, ExtensionTrustState::unsigned_package,
                   identity, launch),
               "explicit Browser launch decision should register runtime") ||
      !require(!authority.register_runtime(
                   manifest, ExtensionTrustState::verified_signature,
                   identity, launch),
               "runtime instance identifiers must not be reusable")) {
    return 1;
  }

  // The manifest accepted at registration is immutable runtime authority.
  // A same-ID manifest cannot be substituted later to broaden permissions.
  auto broader_manifest = manifest;
  broader_manifest.requested_permissions.push_back("network.request");
  ExtensionPermissionLedger substitution_ledger;
  if (!require(substitution_ledger.add(make_always_lease(
                   10, ExtensionPermission::network_requests,
                   "profile-personal", false)),
               "substitution grant fixture should be valid")) {
    return 1;
  }
  auto broader_request =
      make_request(manifest.id, "profile-personal", "runtime-1", 900);
  broader_request.permission = ExtensionPermission::network_requests;
  if (!require(validate_extension_manifest(broader_manifest).accepted(),
               "same-ID broader manifest fixture should itself be valid") ||
      !require(!authority.issue_capability(substitution_ledger, broader_request)
                    .has_value(),
               "runtime must reject permission absent from registered manifest")) {
    return 1;
  }

  ExtensionPermissionLedger ledger;
  if (!require(ledger.add(make_always_lease(
                   1, ExtensionPermission::read_tabs, "profile-personal", false)),
               "normal profile grant should be accepted")) {
    return 1;
  }

  auto request = make_request(manifest.id, "profile-personal", "runtime-1", 1000);
  const auto token = authority.issue_capability(ledger, request);
  if (!require(token.has_value(),
               "running runtime with an authorized grant should receive token")) {
    return 1;
  }

  auto wrong_runtime = make_dispatch(request, 1001);
  wrong_runtime.runtime_instance_id = "runtime-other";
  if (!require(!authority.authorize_and_consume(token->id, wrong_runtime),
               "capability token must be runtime-instance bound")) {
    return 1;
  }

  auto wrong_profile = make_dispatch(request, 1001);
  wrong_profile.profile_id = "profile-work";
  if (!require(!authority.authorize_and_consume(token->id, wrong_profile),
               "capability token must be profile bound")) {
    return 1;
  }

  auto allowed_dispatch = make_dispatch(request, 1001);
  if (!require(authority.authorize_and_consume(token->id, allowed_dispatch),
               "exact bound capability should authorize one dispatch") ||
      !require(!authority.authorize_and_consume(token->id, allowed_dispatch),
               "capability token must be single-use")) {
    return 1;
  }

  const auto suspend_token = authority.issue_capability(ledger, request);
  if (!require(suspend_token.has_value(), "second capability should issue") ||
      !require(authority.suspend("runtime-1"), "running runtime should suspend") ||
      !require(!authority.authorize_and_consume(
                   suspend_token->id, make_dispatch(request, 1002)),
               "suspension must revoke outstanding capabilities") ||
      !require(!authority.resume("runtime-1", false),
               "resume must require explicit Browser authority") ||
      !require(authority.resume("runtime-1", true),
               "authorized resume should restore running state")) {
    return 1;
  }

  const auto expiry_token = authority.issue_capability(ledger, request);
  if (!require(expiry_token.has_value(), "expiry capability should issue") ||
      !require(!authority.authorize_and_consume(
                   expiry_token->id,
                   make_dispatch(request,
                                 1000 + kExtensionRuntimeCapabilityMaxMillis)),
               "expired capability must fail closed")) {
    return 1;
  }

  // Private browsing requires both Browser launch authorization and a grant that
  // independently opts into private use.
  ExtensionRuntimeIdentity private_identity{
      .extension_id = manifest.id,
      .profile_id = "profile-private",
      .runtime_instance_id = "runtime-private",
      .private_browsing = true,
  };
  if (!require(!authority.register_runtime(
                   manifest, ExtensionTrustState::signed_package,
                   private_identity,
                   ExtensionRuntimeLaunchDecision{
                       .browser_launch_authorized = true,
                       .private_browsing_authorized = false,
                   }),
               "private runtime must require explicit private authorization") ||
      !require(authority.register_runtime(
                   manifest, ExtensionTrustState::signed_package,
                   private_identity,
                   ExtensionRuntimeLaunchDecision{
                       .browser_launch_authorized = true,
                       .private_browsing_authorized = true,
                   }),
               "private runtime should register only after explicit approval")) {
    return 1;
  }

  ExtensionPermissionLedger private_denied_ledger;
  if (!require(private_denied_ledger.add(make_always_lease(
                   2, ExtensionPermission::read_tabs, "profile-private", false)),
               "private-denied grant fixture should be valid")) {
    return 1;
  }
  auto private_request = make_request(
      manifest.id, "profile-private", "runtime-private", 2000, true);
  if (!require(!authority.issue_capability(
                   private_denied_ledger, private_request)
                    .has_value(),
               "normal grant must not leak into private browsing")) {
    return 1;
  }

  ExtensionPermissionLedger private_allowed_ledger;
  if (!require(private_allowed_ledger.add(make_always_lease(
                   3, ExtensionPermission::read_tabs, "profile-private", true)),
               "private-enabled grant fixture should be valid")) {
    return 1;
  }
  const auto private_token = authority.issue_capability(
      private_allowed_ledger, private_request);
  if (!require(private_token.has_value(),
               "explicit private grant should allow bounded capability") ||
      !require(authority.terminate("runtime-private"),
               "private runtime should terminate") ||
      !require(!authority.authorize_and_consume(
                   private_token->id, make_dispatch(private_request, 2001)),
               "termination must revoke every outstanding runtime capability") ||
      !require(!authority.resume("runtime-private", true),
               "terminated runtime must not be resumable")) {
    return 1;
  }

  const auto final_token = authority.issue_capability(ledger, request);
  if (!require(final_token.has_value(), "final capability should issue") ||
      !require(authority.terminate("runtime-1"),
               "normal runtime should terminate") ||
      !require(!authority.authorize_and_consume(
                   final_token->id, make_dispatch(request, 1003)),
               "terminated runtime must have zero privileged API authority")) {
    return 1;
  }

  const auto* terminated = authority.session("runtime-1");
  if (!require(terminated != nullptr &&
                   terminated->state == ExtensionRuntimeState::terminated &&
                   !extension_manifest_declares_permission(
                       terminated->manifest,
                       ExtensionPermission::network_requests),
               "terminated session should retain original manifest authority")) {
    return 1;
  }

  return 0;
}
