#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "goreecloud/browser/permission_broker.hpp"

using goreecloud::browser::AuthorityDecision;
using goreecloud::browser::AuthorityGate;
using goreecloud::browser::EvaluationContext;
using goreecloud::browser::HostOsPermissionState;
using goreecloud::browser::PermissionBroker;
using goreecloud::browser::PermissionDecision;
using goreecloud::browser::PermissionLifecycleState;
using goreecloud::browser::PermissionRequest;
using goreecloud::browser::PermissionResource;
using goreecloud::browser::PrivacyContext;
using goreecloud::browser::ResourceAuthoritySnapshot;
using goreecloud::browser::RevalidationContext;
using goreecloud::browser::UserPermissionDecision;

namespace {

PermissionRequest request(
    std::vector<PermissionResource> resources = {PermissionResource::camera},
    PrivacyContext privacy_context = PrivacyContext::normal) {
  return PermissionRequest{
      .request_id = "req-1",
      .profile_id = "profile-personal",
      .privacy_context_id = "ctx-normal",
      .privacy_context = privacy_context,
      .tab_owner_id = "tab-1",
      .top_level_origin = "https://example.test",
      .requesting_origin = "https://example.test",
      .resources = std::move(resources),
      .user_gesture = true,
      .created_at_millis = 1000,
      .expires_at_millis = 5000,
      .contract_version = 1,
  };
}

RevalidationContext current(const PermissionRequest& value) {
  return RevalidationContext{
      .profile_id = value.profile_id,
      .privacy_context_id = value.privacy_context_id,
      .privacy_context = value.privacy_context,
      .tab_owner_id = value.tab_owner_id,
      .top_level_origin = value.top_level_origin,
      .requesting_origin = value.requesting_origin,
  };
}

ResourceAuthoritySnapshot snapshot(
    PermissionResource resource,
    HostOsPermissionState os = HostOsPermissionState::granted) {
  return ResourceAuthoritySnapshot{
      .resource = resource,
      .host_os = os,
      .goreecloud_policy = AuthorityGate{
          .required = true,
          .decision = AuthorityDecision::allow,
      },
      .privacy_shield = AuthorityGate{
          .required = true,
          .decision = AuthorityDecision::allow,
      },
      .wardveil_security = AuthorityGate{
          .required = true,
          .decision = AuthorityDecision::allow,
      },
  };
}

EvaluationContext context_for(
    const PermissionRequest& value,
    std::vector<ResourceAuthoritySnapshot> resources) {
  return EvaluationContext{
      .current = current(value),
      .resources = std::move(resources),
  };
}

}  // namespace

int main() {
  assert(goreecloud::browser::parse_permission_resource("camera").has_value());
  assert(!goreecloud::browser::parse_permission_resource("unknown").has_value());
  assert(goreecloud::browser::canonical_web_origin("https://example.test"));
  assert(goreecloud::browser::canonical_web_origin("https://example.test/"));
  assert(!goreecloud::browser::canonical_web_origin("https://user@example.test"));
  assert(!goreecloud::browser::canonical_web_origin("https://example.test/path"));
  assert(!goreecloud::browser::canonical_web_origin("file:///tmp/example"));

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    assert(!broker.receive(value));
    const auto evaluation =
        broker.evaluate("req-1", context_for(value, {snapshot(PermissionResource::camera)}),
                        2000);
    assert(evaluation.size() == 1);
    assert(evaluation.front().decision == PermissionDecision::pending_user_decision);
    assert(!evaluation.front().engine_grant_allowed);
    assert(broker.state("req-1") == PermissionLifecycleState::user_decision);
  }

  {
    PermissionBroker broker;
    auto value = request({PermissionResource::camera,
                          PermissionResource::microphone});
    assert(broker.receive(value));
    auto camera = snapshot(PermissionResource::camera);
    auto microphone = snapshot(PermissionResource::microphone);
    microphone.privacy_shield.decision = AuthorityDecision::deny;
    const auto evaluation =
        broker.evaluate("req-1", context_for(value, {camera, microphone}), 2000);
    assert(evaluation.size() == 2);
    const auto camera_result = std::find_if(
        evaluation.begin(), evaluation.end(), [](const auto& result) {
          return result.resource == PermissionResource::camera;
        });
    const auto microphone_result = std::find_if(
        evaluation.begin(), evaluation.end(), [](const auto& result) {
          return result.resource == PermissionResource::microphone;
        });
    assert(camera_result != evaluation.end());
    assert(microphone_result != evaluation.end());
    assert(camera_result->decision == PermissionDecision::pending_user_decision);
    assert(microphone_result->decision == PermissionDecision::blocked_policy);
    assert(!microphone_result->engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    auto security_block = snapshot(PermissionResource::camera);
    security_block.wardveil_security.decision = AuthorityDecision::deny;
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value, {security_block}), 2000);
    assert(result.decision == PermissionDecision::blocked_security);
    assert(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value, {snapshot(PermissionResource::camera)}), 2000);
    assert(result.decision == PermissionDecision::allow_once);
    assert(result.engine_grant_allowed);
    assert(!result.persistent_store_allowed);
    assert(broker.state("req-1") == PermissionLifecycleState::completed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_persistent,
        context_for(value, {snapshot(PermissionResource::camera)}), 2000);
    assert(result.decision == PermissionDecision::allow_persistent);
    assert(result.engine_grant_allowed);
    assert(result.persistent_store_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request({PermissionResource::geolocation},
                         PrivacyContext::private_browsing);
    value.privacy_context_id = "ctx-private";
    assert(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::geolocation,
        UserPermissionDecision::allow_persistent,
        context_for(value, {snapshot(PermissionResource::geolocation)}), 2000);
    assert(result.decision == PermissionDecision::error_fail_closed);
    assert(!result.engine_grant_allowed);
    assert(!result.persistent_store_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    auto changed = context_for(value, {snapshot(PermissionResource::camera)});
    changed.current.top_level_origin = "https://navigated.example";
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once, changed, 2000);
    assert(result.decision == PermissionDecision::cancelled);
    assert(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value, {snapshot(PermissionResource::camera)}), 5000);
    assert(result.decision == PermissionDecision::expired);
    assert(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value,
                    {snapshot(PermissionResource::camera,
                              HostOsPermissionState::denied_requestable)}),
        2000);
    assert(result.decision == PermissionDecision::blocked_os);
    assert(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    assert(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::deny_session,
        context_for(value, {snapshot(PermissionResource::camera)}), 2000);
    assert(result.decision == PermissionDecision::deny_session);
    assert(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto first = request();
    auto second = request({PermissionResource::microphone});
    second.request_id = "req-2";
    assert(broker.receive(first));
    assert(broker.receive(second));
    assert(broker.close_context("ctx-normal") == 2);
    assert(broker.state("req-1") == PermissionLifecycleState::completed);
    assert(broker.state("req-2") == PermissionLifecycleState::completed);
  }

  {
    PermissionBroker broker;
    auto invalid = request();
    invalid.resources.push_back(PermissionResource::camera);
    assert(!broker.receive(invalid));
    invalid = request();
    invalid.requesting_origin = "https://example.test/path";
    assert(!broker.receive(invalid));
  }

  return 0;
}
