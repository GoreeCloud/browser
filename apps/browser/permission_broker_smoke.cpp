#include <algorithm>
#include <cassert>

#include "goreecloud/browser/permission_broker.hpp"

using namespace goreecloud::browser;

namespace {

PermissionRequest base_request() {
  return PermissionRequest{
      .request_id = "permission-1",
      .profile_id = "profile-personal",
      .privacy_context_id = "context-normal",
      .privacy_context = BrowserPrivacyContext::normal,
      .owner_id = "tab-1",
      .top_level_origin = "https://example.org",
      .requesting_origin = "https://example.org",
      .created_at_millis = 100,
      .expires_at_millis = 10'000,
      .resources = {
          PermissionResourceInput{
              .resource = PermissionResource::camera,
              .host_state = HostPermissionState::granted,
              .privacy_shield = AuthorityDecisionState::allowed,
              .wardveil = AuthorityDecisionState::allowed,
              .user_decision = PermissionUserDecision::allow_once,
          },
      },
  };
}

PermissionEvaluationContext active_context() {
  return PermissionEvaluationContext{
      .now_millis = 500,
      .owner_active = true,
      .privacy_context_active = true,
      .origins_unchanged = true,
  };
}

const PermissionResourceResult& resource(
    const PermissionEvaluationResult& result,
    PermissionResource target) {
  const auto it = std::find_if(
      result.resources.begin(), result.resources.end(),
      [target](const PermissionResourceResult& candidate) {
        return candidate.resource == target;
      });
  assert(it != result.resources.end());
  return *it;
}

}  // namespace

int main() {
  {
    const auto result = evaluate_permission_request(base_request(), active_context());
    assert(result.status == PermissionRequestStatus::completed);
    assert(result.all_terminal());
    assert(result.all_engine_granted());
    assert(resource(result, PermissionResource::camera).decision ==
           PermissionDecisionCode::allow_once);
    assert(!resource(result, PermissionResource::camera).persistence_allowed);
  }

  {
    auto request = base_request();
    request.resources.front().user_decision.reset();
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.status == PermissionRequestStatus::pending);
    assert(!result.all_terminal());
    assert(!result.any_engine_grant());
    assert(resource(result, PermissionResource::camera).reason ==
           PermissionDecisionReason::user_decision_pending);
  }

  {
    auto request = base_request();
    request.resources.front().privacy_shield = AuthorityDecisionState::unavailable;
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.status == PermissionRequestStatus::completed);
    assert(!result.any_engine_grant());
    assert(resource(result, PermissionResource::camera).decision ==
           PermissionDecisionCode::unavailable);
    assert(resource(result, PermissionResource::camera).reason ==
           PermissionDecisionReason::privacy_shield_unavailable);
  }

  {
    auto request = base_request();
    request.resources.front().wardveil = AuthorityDecisionState::denied;
    const auto result = evaluate_permission_request(request, active_context());
    assert(!result.any_engine_grant());
    assert(resource(result, PermissionResource::camera).decision ==
           PermissionDecisionCode::blocked_security);
  }

  {
    auto request = base_request();
    request.resources.front().host_state = HostPermissionState::denied_no_reprompt;
    const auto result = evaluate_permission_request(request, active_context());
    assert(!result.any_engine_grant());
    assert(resource(result, PermissionResource::camera).decision ==
           PermissionDecisionCode::blocked_os);
  }

  {
    auto request = base_request();
    request.privacy_context = BrowserPrivacyContext::private_browsing;
    request.privacy_context_id = "context-private";
    request.resources.front().user_decision =
        PermissionUserDecision::allow_persistent;
    const auto result = evaluate_permission_request(request, active_context());
    assert(!result.any_engine_grant());
    assert(resource(result, PermissionResource::camera).decision ==
           PermissionDecisionCode::blocked_policy);
    assert(resource(result, PermissionResource::camera).reason ==
           PermissionDecisionReason::persistent_private_decision_forbidden);
  }

  {
    auto request = base_request();
    request.privacy_context = BrowserPrivacyContext::isolated_private;
    request.privacy_context_id = "context-isolated-private";
    request.resources.front().user_decision =
        PermissionUserDecision::allow_session;
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.any_engine_grant());
    assert(!resource(result, PermissionResource::camera).persistence_allowed);
  }

  {
    auto request = base_request();
    request.resources.front().user_decision =
        PermissionUserDecision::allow_persistent;
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.any_engine_grant());
    assert(resource(result, PermissionResource::camera).persistence_allowed);
  }

  {
    auto request = base_request();
    request.resources.push_back(PermissionResourceInput{
        .resource = PermissionResource::microphone,
        .host_state = HostPermissionState::unavailable,
        .privacy_shield = AuthorityDecisionState::allowed,
        .wardveil = AuthorityDecisionState::allowed,
        .user_decision = PermissionUserDecision::allow_once,
    });
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.status == PermissionRequestStatus::completed);
    assert(result.any_engine_grant());
    assert(!result.all_engine_granted());
    assert(resource(result, PermissionResource::camera).engine_allowed);
    assert(!resource(result, PermissionResource::microphone).engine_allowed);
  }

  {
    auto request = base_request();
    request.resources.push_back(request.resources.front());
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.status == PermissionRequestStatus::error_fail_closed);
    assert(!result.any_engine_grant());
  }

  {
    auto request = base_request();
    auto context = active_context();
    context.now_millis = request.expires_at_millis;
    const auto result = evaluate_permission_request(request, context);
    assert(result.status == PermissionRequestStatus::expired);
    assert(resource(result, PermissionResource::camera).decision ==
           PermissionDecisionCode::expired);
  }

  {
    auto context = active_context();
    context.owner_active = false;
    const auto result = evaluate_permission_request(base_request(), context);
    assert(result.status == PermissionRequestStatus::cancelled);
    assert(resource(result, PermissionResource::camera).reason ==
           PermissionDecisionReason::owner_inactive);
  }

  {
    auto context = active_context();
    context.origins_unchanged = false;
    const auto result = evaluate_permission_request(base_request(), context);
    assert(result.status == PermissionRequestStatus::cancelled);
    assert(resource(result, PermissionResource::camera).reason ==
           PermissionDecisionReason::origin_changed);
  }

  {
    auto request = base_request();
    request.top_level_origin = "file:///tmp/private";
    const auto result = evaluate_permission_request(request, active_context());
    assert(result.status == PermissionRequestStatus::error_fail_closed);
    assert(!result.any_engine_grant());
  }

  return 0;
}
