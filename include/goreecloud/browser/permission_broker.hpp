#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace goreecloud::browser {

enum class BrowserPrivacyContext {
  normal,
  private_browsing,
  isolated_private,
};

enum class PermissionResource {
  camera,
  microphone,
  geolocation,
  protected_media,
  midi_sysex,
};

enum class HostPermissionState {
  granted,
  denied_requestable,
  denied_no_reprompt,
  restricted,
  unavailable,
  error,
};

enum class AuthorityDecisionState {
  not_required,
  allowed,
  denied,
  unavailable,
  invalid,
};

enum class PermissionUserDecision {
  allow_once,
  allow_session,
  allow_persistent,
  deny_once,
  deny_session,
  deny_persistent,
};

enum class PermissionDecisionCode {
  pending,
  allow_once,
  allow_session,
  allow_persistent,
  deny_once,
  deny_session,
  deny_persistent,
  blocked_policy,
  blocked_security,
  blocked_os,
  unavailable,
  cancelled,
  expired,
  error_fail_closed,
};

enum class PermissionDecisionReason {
  none,
  user_decision_pending,
  persistent_private_decision_forbidden,
  privacy_shield_denied,
  privacy_shield_unavailable,
  privacy_shield_invalid,
  wardveil_denied,
  wardveil_unavailable,
  wardveil_invalid,
  host_permission_denied_requestable,
  host_permission_denied_no_reprompt,
  host_permission_restricted,
  host_permission_unavailable,
  host_permission_error,
  request_invalid,
  request_expired,
  owner_inactive,
  privacy_context_inactive,
  origin_changed,
};

enum class PermissionLifecycleState {
  received,
  validating,
  context_bound,
  policy_evaluation,
  os_capability_check,
  user_decision,
  resolving_engine,
  completed,
  cancelled,
  expired,
  failed,
};

enum class PermissionRequestStatus {
  pending,
  completed,
  cancelled,
  expired,
  error_fail_closed,
};

struct PermissionResourceInput {
  PermissionResource resource{PermissionResource::camera};
  HostPermissionState host_state{HostPermissionState::unavailable};
  AuthorityDecisionState privacy_shield{AuthorityDecisionState::not_required};
  AuthorityDecisionState wardveil{AuthorityDecisionState::not_required};
  std::optional<PermissionUserDecision> user_decision;
};

struct PermissionRequest {
  std::string request_id;
  std::string profile_id;
  std::string privacy_context_id;
  BrowserPrivacyContext privacy_context{BrowserPrivacyContext::normal};
  std::string owner_id;
  std::string top_level_origin;
  std::string requesting_origin;
  std::uint64_t created_at_millis{0};
  std::uint64_t expires_at_millis{0};
  std::vector<PermissionResourceInput> resources;
};

struct PermissionEvaluationContext {
  std::uint64_t now_millis{0};
  bool owner_active{true};
  bool privacy_context_active{true};
  bool origins_unchanged{true};
};

struct PermissionResourceResult {
  PermissionResource resource{PermissionResource::camera};
  PermissionDecisionCode decision{PermissionDecisionCode::pending};
  PermissionDecisionReason reason{PermissionDecisionReason::none};
  PermissionLifecycleState lifecycle{PermissionLifecycleState::received};
  bool terminal{false};
  bool engine_allowed{false};
  bool persistence_allowed{false};
};

struct PermissionEvaluationResult {
  PermissionRequestStatus status{PermissionRequestStatus::error_fail_closed};
  PermissionLifecycleState lifecycle{PermissionLifecycleState::failed};
  std::vector<PermissionResourceResult> resources;

  [[nodiscard]] bool all_terminal() const noexcept {
    return std::all_of(resources.begin(), resources.end(),
                       [](const PermissionResourceResult& result) {
                         return result.terminal;
                       });
  }

  [[nodiscard]] bool any_engine_grant() const noexcept {
    return std::any_of(resources.begin(), resources.end(),
                       [](const PermissionResourceResult& result) {
                         return result.engine_allowed;
                       });
  }

  [[nodiscard]] bool all_engine_granted() const noexcept {
    return !resources.empty() &&
           std::all_of(resources.begin(), resources.end(),
                       [](const PermissionResourceResult& result) {
                         return result.engine_allowed;
                       });
  }
};

inline bool permission_string_has_control_or_space(std::string_view value) {
  return std::any_of(value.begin(), value.end(), [](unsigned char character) {
    return std::iscntrl(character) != 0 || std::isspace(character) != 0;
  });
}

inline bool valid_permission_origin(std::string_view origin) {
  if (origin.empty() || origin.size() > 2048 ||
      permission_string_has_control_or_space(origin)) {
    return false;
  }
  return origin.starts_with("https://") || origin.starts_with("http://");
}

inline bool permission_request_is_structurally_valid(
    const PermissionRequest& request) {
  if (request.request_id.empty() || request.profile_id.empty() ||
      request.privacy_context_id.empty() || request.owner_id.empty() ||
      !valid_permission_origin(request.top_level_origin) ||
      !valid_permission_origin(request.requesting_origin) ||
      request.created_at_millis == 0 ||
      request.expires_at_millis <= request.created_at_millis ||
      request.resources.empty() || request.resources.size() > 16) {
    return false;
  }

  std::vector<PermissionResource> seen;
  seen.reserve(request.resources.size());
  for (const auto& resource : request.resources) {
    if (std::find(seen.begin(), seen.end(), resource.resource) != seen.end()) {
      return false;
    }
    seen.push_back(resource.resource);
  }
  return true;
}

inline PermissionResourceResult terminal_permission_result(
    PermissionResource resource,
    PermissionDecisionCode decision,
    PermissionDecisionReason reason,
    PermissionLifecycleState lifecycle,
    bool engine_allowed = false,
    bool persistence_allowed = false) {
  return PermissionResourceResult{
      .resource = resource,
      .decision = decision,
      .reason = reason,
      .lifecycle = lifecycle,
      .terminal = true,
      .engine_allowed = engine_allowed,
      .persistence_allowed = persistence_allowed,
  };
}

inline PermissionResourceResult evaluate_permission_resource(
    const PermissionResourceInput& input,
    BrowserPrivacyContext privacy_context) {
  switch (input.privacy_shield) {
    case AuthorityDecisionState::denied:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::blocked_policy,
          PermissionDecisionReason::privacy_shield_denied,
          PermissionLifecycleState::policy_evaluation);
    case AuthorityDecisionState::unavailable:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::unavailable,
          PermissionDecisionReason::privacy_shield_unavailable,
          PermissionLifecycleState::policy_evaluation);
    case AuthorityDecisionState::invalid:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::error_fail_closed,
          PermissionDecisionReason::privacy_shield_invalid,
          PermissionLifecycleState::failed);
    case AuthorityDecisionState::not_required:
    case AuthorityDecisionState::allowed:
      break;
  }

  switch (input.wardveil) {
    case AuthorityDecisionState::denied:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::blocked_security,
          PermissionDecisionReason::wardveil_denied,
          PermissionLifecycleState::policy_evaluation);
    case AuthorityDecisionState::unavailable:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::unavailable,
          PermissionDecisionReason::wardveil_unavailable,
          PermissionLifecycleState::policy_evaluation);
    case AuthorityDecisionState::invalid:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::error_fail_closed,
          PermissionDecisionReason::wardveil_invalid,
          PermissionLifecycleState::failed);
    case AuthorityDecisionState::not_required:
    case AuthorityDecisionState::allowed:
      break;
  }

  switch (input.host_state) {
    case HostPermissionState::granted:
      break;
    case HostPermissionState::denied_requestable:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::blocked_os,
          PermissionDecisionReason::host_permission_denied_requestable,
          PermissionLifecycleState::os_capability_check);
    case HostPermissionState::denied_no_reprompt:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::blocked_os,
          PermissionDecisionReason::host_permission_denied_no_reprompt,
          PermissionLifecycleState::os_capability_check);
    case HostPermissionState::restricted:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::blocked_os,
          PermissionDecisionReason::host_permission_restricted,
          PermissionLifecycleState::os_capability_check);
    case HostPermissionState::unavailable:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::unavailable,
          PermissionDecisionReason::host_permission_unavailable,
          PermissionLifecycleState::os_capability_check);
    case HostPermissionState::error:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::error_fail_closed,
          PermissionDecisionReason::host_permission_error,
          PermissionLifecycleState::failed);
  }

  if (!input.user_decision.has_value()) {
    return PermissionResourceResult{
        .resource = input.resource,
        .decision = PermissionDecisionCode::pending,
        .reason = PermissionDecisionReason::user_decision_pending,
        .lifecycle = PermissionLifecycleState::user_decision,
        .terminal = false,
        .engine_allowed = false,
        .persistence_allowed = false,
    };
  }

  const bool private_context = privacy_context != BrowserPrivacyContext::normal;
  if (private_context &&
      (*input.user_decision == PermissionUserDecision::allow_persistent ||
       *input.user_decision == PermissionUserDecision::deny_persistent)) {
    return terminal_permission_result(
        input.resource, PermissionDecisionCode::blocked_policy,
        PermissionDecisionReason::persistent_private_decision_forbidden,
        PermissionLifecycleState::completed);
  }

  switch (*input.user_decision) {
    case PermissionUserDecision::allow_once:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::allow_once,
          PermissionDecisionReason::none,
          PermissionLifecycleState::completed, true, false);
    case PermissionUserDecision::allow_session:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::allow_session,
          PermissionDecisionReason::none,
          PermissionLifecycleState::completed, true, false);
    case PermissionUserDecision::allow_persistent:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::allow_persistent,
          PermissionDecisionReason::none,
          PermissionLifecycleState::completed, true, true);
    case PermissionUserDecision::deny_once:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::deny_once,
          PermissionDecisionReason::none,
          PermissionLifecycleState::completed);
    case PermissionUserDecision::deny_session:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::deny_session,
          PermissionDecisionReason::none,
          PermissionLifecycleState::completed);
    case PermissionUserDecision::deny_persistent:
      return terminal_permission_result(
          input.resource, PermissionDecisionCode::deny_persistent,
          PermissionDecisionReason::none,
          PermissionLifecycleState::completed, false, true);
  }

  return terminal_permission_result(
      input.resource, PermissionDecisionCode::error_fail_closed,
      PermissionDecisionReason::request_invalid,
      PermissionLifecycleState::failed);
}

inline PermissionEvaluationResult terminal_request_result(
    const PermissionRequest& request,
    PermissionRequestStatus status,
    PermissionLifecycleState lifecycle,
    PermissionDecisionCode decision,
    PermissionDecisionReason reason) {
  PermissionEvaluationResult result{
      .status = status,
      .lifecycle = lifecycle,
  };
  result.resources.reserve(request.resources.size());
  for (const auto& input : request.resources) {
    result.resources.push_back(terminal_permission_result(
        input.resource, decision, reason, lifecycle));
  }
  return result;
}

inline PermissionEvaluationResult evaluate_permission_request(
    const PermissionRequest& request,
    const PermissionEvaluationContext& context) {
  if (!permission_request_is_structurally_valid(request)) {
    return terminal_request_result(
        request, PermissionRequestStatus::error_fail_closed,
        PermissionLifecycleState::failed,
        PermissionDecisionCode::error_fail_closed,
        PermissionDecisionReason::request_invalid);
  }

  if (context.now_millis >= request.expires_at_millis) {
    return terminal_request_result(
        request, PermissionRequestStatus::expired,
        PermissionLifecycleState::expired,
        PermissionDecisionCode::expired,
        PermissionDecisionReason::request_expired);
  }

  if (!context.owner_active) {
    return terminal_request_result(
        request, PermissionRequestStatus::cancelled,
        PermissionLifecycleState::cancelled,
        PermissionDecisionCode::cancelled,
        PermissionDecisionReason::owner_inactive);
  }
  if (!context.privacy_context_active) {
    return terminal_request_result(
        request, PermissionRequestStatus::cancelled,
        PermissionLifecycleState::cancelled,
        PermissionDecisionCode::cancelled,
        PermissionDecisionReason::privacy_context_inactive);
  }
  if (!context.origins_unchanged) {
    return terminal_request_result(
        request, PermissionRequestStatus::cancelled,
        PermissionLifecycleState::cancelled,
        PermissionDecisionCode::cancelled,
        PermissionDecisionReason::origin_changed);
  }

  PermissionEvaluationResult result{
      .status = PermissionRequestStatus::completed,
      .lifecycle = PermissionLifecycleState::completed,
  };
  result.resources.reserve(request.resources.size());

  bool pending = false;
  bool failed = false;
  for (const auto& input : request.resources) {
    auto resource = evaluate_permission_resource(input, request.privacy_context);
    pending = pending || !resource.terminal;
    failed = failed ||
             resource.decision == PermissionDecisionCode::error_fail_closed;
    result.resources.push_back(resource);
  }

  if (failed) {
    result.status = PermissionRequestStatus::error_fail_closed;
    result.lifecycle = PermissionLifecycleState::failed;
  } else if (pending) {
    result.status = PermissionRequestStatus::pending;
    result.lifecycle = PermissionLifecycleState::user_decision;
  }

  return result;
}

}  // namespace goreecloud::browser
