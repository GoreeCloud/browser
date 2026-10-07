#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
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

void require(bool condition) {
  if (!condition) std::abort();
}

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
  require(goreecloud::browser::parse_permission_resource("camera").has_value());
  require(!goreecloud::browser::parse_permission_resource("unknown").has_value());
  require(goreecloud::browser::canonical_web_origin("https://example.test"));
  require(goreecloud::browser::canonical_web_origin("https://example.test/"));
  require(!goreecloud::browser::canonical_web_origin("https://user@example.test"));
  require(!goreecloud::browser::canonical_web_origin("https://example.test/path"));
  require(!goreecloud::browser::canonical_web_origin("file:///tmp/example"));

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    require(!broker.receive(value));
    const auto evaluation =
        broker.evaluate("req-1", context_for(value, {snapshot(PermissionResource::camera)}),
                        2000);
    require(evaluation.size() == 1);
    require(evaluation.front().decision == PermissionDecision::pending_user_decision);
    require(!evaluation.front().engine_grant_allowed);
    require(broker.state("req-1") == PermissionLifecycleState::user_decision);
  }

  {
    PermissionBroker broker;
    auto value = request({PermissionResource::camera,
                          PermissionResource::microphone});
    require(broker.receive(value));
    auto camera = snapshot(PermissionResource::camera);
    auto microphone = snapshot(PermissionResource::microphone);
    microphone.privacy_shield.decision = AuthorityDecision::deny;
    const auto evaluation =
        broker.evaluate("req-1", context_for(value, {camera, microphone}), 2000);
    require(evaluation.size() == 2);
    const auto camera_result = std::find_if(
        evaluation.begin(), evaluation.end(), [](const auto& result) {
          return result.resource == PermissionResource::camera;
        });
    const auto microphone_result = std::find_if(
        evaluation.begin(), evaluation.end(), [](const auto& result) {
          return result.resource == PermissionResource::microphone;
        });
    require(camera_result != evaluation.end());
    require(microphone_result != evaluation.end());
    require(camera_result->decision == PermissionDecision::pending_user_decision);
    require(microphone_result->decision == PermissionDecision::blocked_policy);
    require(!microphone_result->engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request({PermissionResource::camera,
                          PermissionResource::microphone});
    require(broker.receive(value));
    const auto ctx = context_for(
        value,
        {snapshot(PermissionResource::camera),
         snapshot(PermissionResource::microphone)});

    const auto camera = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once, ctx, 2000);
    require(camera.decision == PermissionDecision::allow_once);
    require(camera.engine_grant_allowed);
    require(broker.state("req-1") == PermissionLifecycleState::user_decision);

    const auto microphone = broker.apply_user_decision(
        "req-1", PermissionResource::microphone,
        UserPermissionDecision::deny_once, ctx, 2000);
    require(microphone.decision == PermissionDecision::deny_once);
    require(!microphone.engine_grant_allowed);
    require(broker.state("req-1") == PermissionLifecycleState::completed);
  }

  {
    PermissionBroker broker;
    auto value = request({PermissionResource::camera,
                          PermissionResource::microphone});
    require(broker.receive(value));
    auto camera = snapshot(PermissionResource::camera);
    auto microphone = snapshot(PermissionResource::microphone);
    microphone.privacy_shield.decision = AuthorityDecision::deny;
    const auto ctx = context_for(value, {camera, microphone});

    const auto evaluation = broker.evaluate("req-1", ctx, 2000);
    require(evaluation.size() == 2);
    require(broker.state("req-1") == PermissionLifecycleState::user_decision);

    const auto camera_result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once, ctx, 2000);
    require(camera_result.decision == PermissionDecision::allow_once);
    require(camera_result.engine_grant_allowed);
    require(broker.state("req-1") == PermissionLifecycleState::completed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    auto security_block = snapshot(PermissionResource::camera);
    security_block.wardveil_security.decision = AuthorityDecision::deny;
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value, {security_block}), 2000);
    require(result.decision == PermissionDecision::blocked_security);
    require(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value, {snapshot(PermissionResource::camera)}), 2000);
    require(result.decision == PermissionDecision::allow_once);
    require(result.engine_grant_allowed);
    require(!result.persistent_store_allowed);
    require(broker.state("req-1") == PermissionLifecycleState::completed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_persistent,
        context_for(value, {snapshot(PermissionResource::camera)}), 2000);
    require(result.decision == PermissionDecision::allow_persistent);
    require(result.engine_grant_allowed);
    require(result.persistent_store_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request({PermissionResource::geolocation},
                         PrivacyContext::private_browsing);
    value.privacy_context_id = "ctx-private";
    require(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::geolocation,
        UserPermissionDecision::allow_persistent,
        context_for(value, {snapshot(PermissionResource::geolocation)}), 2000);
    require(result.decision == PermissionDecision::error_fail_closed);
    require(!result.engine_grant_allowed);
    require(!result.persistent_store_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    auto changed = context_for(value, {snapshot(PermissionResource::camera)});
    changed.current.top_level_origin = "https://navigated.example";
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once, changed, 2000);
    require(result.decision == PermissionDecision::cancelled);
    require(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value, {snapshot(PermissionResource::camera)}), 5000);
    require(result.decision == PermissionDecision::expired);
    require(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::allow_once,
        context_for(value,
                    {snapshot(PermissionResource::camera,
                              HostOsPermissionState::denied_requestable)}),
        2000);
    require(result.decision == PermissionDecision::blocked_os);
    require(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto value = request();
    require(broker.receive(value));
    const auto result = broker.apply_user_decision(
        "req-1", PermissionResource::camera,
        UserPermissionDecision::deny_session,
        context_for(value, {snapshot(PermissionResource::camera)}), 2000);
    require(result.decision == PermissionDecision::deny_session);
    require(!result.engine_grant_allowed);
  }

  {
    PermissionBroker broker;
    auto first = request();
    auto second = request({PermissionResource::microphone});
    second.request_id = "req-2";
    require(broker.receive(first));
    require(broker.receive(second));
    require(broker.close_context("ctx-normal") == 2);
    require(broker.state("req-1") == PermissionLifecycleState::completed);
    require(broker.state("req-2") == PermissionLifecycleState::completed);
  }

  {
    PermissionBroker broker;
    auto invalid = request();
    invalid.resources.push_back(PermissionResource::camera);
    require(!broker.receive(invalid));
    invalid = request();
    invalid.requesting_origin = "https://example.test/path";
    require(!broker.receive(invalid));
  }

  return 0;
}
