#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace goreecloud::browser {

enum class PrivacyContext {
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

inline constexpr std::string_view permission_resource_name(
    PermissionResource resource) {
  switch (resource) {
    case PermissionResource::camera:
      return "camera";
    case PermissionResource::microphone:
      return "microphone";
    case PermissionResource::geolocation:
      return "geolocation";
    case PermissionResource::protected_media:
      return "protected_media";
    case PermissionResource::midi_sysex:
      return "midi_sysex";
  }
  return {};
}

inline std::optional<PermissionResource> parse_permission_resource(
    std::string_view value) {
  constexpr PermissionResource resources[] = {
      PermissionResource::camera,
      PermissionResource::microphone,
      PermissionResource::geolocation,
      PermissionResource::protected_media,
      PermissionResource::midi_sysex,
  };
  for (const auto resource : resources) {
    if (permission_resource_name(resource) == value) {
      return resource;
    }
  }
  return std::nullopt;
}

enum class HostOsPermissionState {
  granted,
  denied_requestable,
  denied_no_reprompt,
  restricted,
  unavailable,
  error,
};

enum class AuthorityDecision {
  not_required,
  allow,
  deny,
  unavailable,
  invalid,
};

struct AuthorityGate {
  bool required{false};
  AuthorityDecision decision{AuthorityDecision::not_required};
};

struct PermissionRequest {
  std::string request_id;
  std::string profile_id;
  std::string privacy_context_id;
  PrivacyContext privacy_context{PrivacyContext::normal};
  std::string tab_owner_id;
  std::string top_level_origin;
  std::string requesting_origin;
  std::vector<PermissionResource> resources;
  bool user_gesture{false};
  std::int64_t created_at_millis{0};
  std::int64_t expires_at_millis{0};
  std::uint32_t contract_version{1};
};

struct RevalidationContext {
  std::string profile_id;
  std::string privacy_context_id;
  PrivacyContext privacy_context{PrivacyContext::normal};
  std::string tab_owner_id;
  std::string top_level_origin;
  std::string requesting_origin;
};

struct ResourceAuthoritySnapshot {
  PermissionResource resource{PermissionResource::camera};
  HostOsPermissionState host_os{HostOsPermissionState::unavailable};
  AuthorityGate goreecloud_policy;
  AuthorityGate privacy_shield;
  AuthorityGate wardveil_security;
};

struct EvaluationContext {
  RevalidationContext current;
  std::vector<ResourceAuthoritySnapshot> resources;
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
};

enum class UserPermissionDecision {
  allow_once,
  allow_session,
  allow_persistent,
  deny_once,
  deny_session,
  deny_persistent,
};

enum class PermissionDecision {
  pending_user_decision,
  allow_once,
  allow_session,
  allow_persistent,
  deny_once,
  deny_session,
  deny_persistent,
  cancelled,
  expired,
  blocked_policy,
  blocked_security,
  blocked_os,
  unavailable,
  error_fail_closed,
};

struct PermissionResolution {
  PermissionResource resource{PermissionResource::camera};
  PermissionDecision decision{PermissionDecision::error_fail_closed};
  bool engine_grant_allowed{false};
  bool persistent_store_allowed{false};
  std::string reason;
};

inline bool permission_text_safe(std::string_view value,
                                 std::size_t max_length) {
  if (value.empty() || value.size() > max_length) {
    return false;
  }
  return std::none_of(value.begin(), value.end(), [](unsigned char character) {
    return std::iscntrl(character) != 0;
  });
}

inline bool canonical_web_origin(std::string_view value) {
  if (!permission_text_safe(value, 2048) ||
      value.find(' ') != std::string_view::npos ||
      value.find_first_of("?#") != std::string_view::npos ||
      value.find('@') != std::string_view::npos) {
    return false;
  }

  std::size_t scheme_length = 0;
  if (value.starts_with("https://")) {
    scheme_length = 8;
  } else if (value.starts_with("http://")) {
    scheme_length = 7;
  } else {
    return false;
  }

  if (value.size() <= scheme_length) {
    return false;
  }

  const auto path = value.find('/', scheme_length);
  if (path != std::string_view::npos && path != value.size() - 1) {
    return false;
  }

  const auto authority =
      value.substr(scheme_length, path == std::string_view::npos
                                     ? std::string_view::npos
                                     : path - scheme_length);
  return !authority.empty() && authority.front() != ':' &&
         authority.back() != ':';
}

inline bool valid_permission_request(const PermissionRequest& request) {
  if (request.contract_version != 1 ||
      !permission_text_safe(request.request_id, 128) ||
      !permission_text_safe(request.profile_id, 128) ||
      !permission_text_safe(request.privacy_context_id, 128) ||
      !permission_text_safe(request.tab_owner_id, 128) ||
      !canonical_web_origin(request.top_level_origin) ||
      !canonical_web_origin(request.requesting_origin) ||
      request.resources.empty() || request.resources.size() > 8 ||
      request.created_at_millis < 0 ||
      request.expires_at_millis <= request.created_at_millis) {
    return false;
  }

  std::unordered_set<int> seen;
  for (const auto resource : request.resources) {
    if (!seen.insert(static_cast<int>(resource)).second) {
      return false;
    }
  }
  return true;
}

inline bool request_matches_current_context(
    const PermissionRequest& request,
    const RevalidationContext& current) {
  return request.profile_id == current.profile_id &&
         request.privacy_context_id == current.privacy_context_id &&
         request.privacy_context == current.privacy_context &&
         request.tab_owner_id == current.tab_owner_id &&
         request.top_level_origin == current.top_level_origin &&
         request.requesting_origin == current.requesting_origin;
}

class PermissionBroker {
 public:
  [[nodiscard]] bool receive(PermissionRequest request) {
    if (!valid_permission_request(request) ||
        requests_.contains(request.request_id)) {
      return false;
    }

    StoredRequest stored;
    stored.request = std::move(request);
    stored.state = PermissionLifecycleState::received;
    requests_.emplace(stored.request.request_id, std::move(stored));
    return true;
  }

  [[nodiscard]] std::optional<PermissionLifecycleState> state(
      std::string_view request_id) const {
    const auto* stored = find(request_id);
    if (stored == nullptr) {
      return std::nullopt;
    }
    return stored->state;
  }

  [[nodiscard]] std::vector<PermissionResolution> evaluate(
      std::string_view request_id,
      const EvaluationContext& context,
      std::int64_t now_millis) {
    auto* stored = find_mutable(request_id);
    if (stored == nullptr || stored->state == PermissionLifecycleState::completed) {
      return {};
    }

    stored->state = PermissionLifecycleState::validating;
    const auto common = validate_live_request(stored->request, context.current,
                                              now_millis);
    if (common.has_value()) {
      resolve_all_unresolved(*stored, common->decision);
      stored->state = PermissionLifecycleState::completed;
      return resolutions_for_all(stored->request, *common);
    }

    stored->state = PermissionLifecycleState::context_bound;
    stored->state = PermissionLifecycleState::policy_evaluation;

    std::vector<PermissionResolution> result;
    result.reserve(stored->request.resources.size());
    for (const auto resource : stored->request.resources) {
      const auto resolved = stored->resolved.find(static_cast<int>(resource));
      if (resolved != stored->resolved.end()) {
        result.push_back(PermissionResolution{
            .resource = resource,
            .decision = resolved->second,
            .engine_grant_allowed = false,
            .persistent_store_allowed = false,
            .reason = "resource-already-resolved",
        });
        continue;
      }

      const auto resolution =
          preflight_resource(stored->request, resource, context);
      if (resolution.decision != PermissionDecision::pending_user_decision) {
        stored->resolved.emplace(static_cast<int>(resource),
                                 resolution.decision);
      }
      result.push_back(resolution);
    }

    stored->state = PermissionLifecycleState::os_capability_check;
    stored->state = all_resources_resolved(*stored)
                        ? PermissionLifecycleState::completed
                        : PermissionLifecycleState::user_decision;
    return result;
  }

  [[nodiscard]] PermissionResolution apply_user_decision(
      std::string_view request_id,
      PermissionResource resource,
      UserPermissionDecision user_decision,
      const EvaluationContext& context,
      std::int64_t now_millis) {
    auto* stored = find_mutable(request_id);
    if (stored == nullptr || stored->state == PermissionLifecycleState::completed) {
      return fail(resource, PermissionDecision::error_fail_closed,
                  "request-unavailable");
    }

    if (stored->resolved.contains(static_cast<int>(resource))) {
      return fail(resource, PermissionDecision::error_fail_closed,
                  "resource-already-resolved");
    }

    if (std::find(stored->request.resources.begin(),
                  stored->request.resources.end(),
                  resource) == stored->request.resources.end()) {
      return fail(resource, PermissionDecision::error_fail_closed,
                  "resource-not-requested");
    }

    const auto common = validate_live_request(stored->request, context.current,
                                              now_millis);
    if (common.has_value()) {
      resolve_all_unresolved(*stored, common->decision);
      stored->state = PermissionLifecycleState::completed;
      return PermissionResolution{
          .resource = resource,
          .decision = common->decision,
          .engine_grant_allowed = false,
          .persistent_store_allowed = false,
          .reason = common->reason,
      };
    }

    if (is_persistent(user_decision) &&
        stored->request.privacy_context != PrivacyContext::normal) {
      stored->resolved.emplace(static_cast<int>(resource),
                               PermissionDecision::error_fail_closed);
      stored->state = all_resources_resolved(*stored)
                          ? PermissionLifecycleState::completed
                          : PermissionLifecycleState::user_decision;
      return fail(resource, PermissionDecision::error_fail_closed,
                  "persistent-private-decision-prohibited");
    }

    if (is_deny(user_decision)) {
      const auto decision = map_user_decision(user_decision);
      stored->resolved.emplace(static_cast<int>(resource), decision);
      stored->state = all_resources_resolved(*stored)
                          ? PermissionLifecycleState::completed
                          : PermissionLifecycleState::user_decision;
      return PermissionResolution{
          .resource = resource,
          .decision = decision,
          .engine_grant_allowed = false,
          .persistent_store_allowed =
              decision == PermissionDecision::deny_persistent &&
              stored->request.privacy_context == PrivacyContext::normal,
          .reason = "user-denied",
      };
    }

    const auto preflight =
        preflight_resource(stored->request, resource, context);
    if (preflight.decision != PermissionDecision::pending_user_decision) {
      stored->resolved.emplace(static_cast<int>(resource),
                               preflight.decision);
      stored->state = all_resources_resolved(*stored)
                          ? PermissionLifecycleState::completed
                          : PermissionLifecycleState::user_decision;
      return preflight;
    }

    stored->state = PermissionLifecycleState::resolving_engine;
    const auto decision = map_user_decision(user_decision);
    stored->resolved.emplace(static_cast<int>(resource), decision);
    stored->state = all_resources_resolved(*stored)
                        ? PermissionLifecycleState::completed
                        : PermissionLifecycleState::user_decision;

    return PermissionResolution{
        .resource = resource,
        .decision = decision,
        .engine_grant_allowed = true,
        .persistent_store_allowed =
            decision == PermissionDecision::allow_persistent &&
            stored->request.privacy_context == PrivacyContext::normal,
        .reason = "explicit-user-allow-after-authority-checks",
    };
  }

  [[nodiscard]] bool cancel(std::string_view request_id) {
    auto* stored = find_mutable(request_id);
    if (stored == nullptr || stored->state == PermissionLifecycleState::completed) {
      return false;
    }
    resolve_all_unresolved(*stored, PermissionDecision::cancelled);
    stored->state = PermissionLifecycleState::completed;
    return true;
  }

  [[nodiscard]] std::size_t close_context(std::string_view context_id) {
    std::size_t cancelled = 0;
    for (auto& [request_id, stored] : requests_) {
      (void)request_id;
      if (stored.state != PermissionLifecycleState::completed &&
          stored.request.privacy_context_id == context_id) {
        resolve_all_unresolved(stored, PermissionDecision::cancelled);
        stored.state = PermissionLifecycleState::completed;
        ++cancelled;
      }
    }
    return cancelled;
  }

 private:
  struct StoredRequest {
    PermissionRequest request;
    PermissionLifecycleState state{PermissionLifecycleState::received};
    std::unordered_map<int, PermissionDecision> resolved;
  };

  struct CommonFailure {
    PermissionDecision decision{PermissionDecision::error_fail_closed};
    std::string reason;
  };

  std::unordered_map<std::string, StoredRequest> requests_;

  [[nodiscard]] const StoredRequest* find(std::string_view request_id) const {
    const auto iterator = requests_.find(std::string{request_id});
    return iterator == requests_.end() ? nullptr : &iterator->second;
  }

  [[nodiscard]] StoredRequest* find_mutable(std::string_view request_id) {
    const auto iterator = requests_.find(std::string{request_id});
    return iterator == requests_.end() ? nullptr : &iterator->second;
  }

  [[nodiscard]] static bool all_resources_resolved(
      const StoredRequest& stored) {
    return stored.resolved.size() == stored.request.resources.size();
  }

  static void resolve_all_unresolved(StoredRequest& stored,
                                     PermissionDecision decision) {
    for (const auto resource : stored.request.resources) {
      stored.resolved.try_emplace(static_cast<int>(resource), decision);
    }
  }

  [[nodiscard]] static std::optional<CommonFailure> validate_live_request(
      const PermissionRequest& request,
      const RevalidationContext& current,
      std::int64_t now_millis) {
    if (now_millis < request.created_at_millis) {
      return CommonFailure{PermissionDecision::error_fail_closed,
                           "clock-regression"};
    }
    if (now_millis >= request.expires_at_millis) {
      return CommonFailure{PermissionDecision::expired, "request-expired"};
    }
    if (!request_matches_current_context(request, current)) {
      return CommonFailure{PermissionDecision::cancelled,
                           "request-context-changed"};
    }
    return std::nullopt;
  }

  [[nodiscard]] static PermissionResolution fail(
      PermissionResource resource,
      PermissionDecision decision,
      std::string reason) {
    return PermissionResolution{
        .resource = resource,
        .decision = decision,
        .engine_grant_allowed = false,
        .persistent_store_allowed = false,
        .reason = std::move(reason),
    };
  }

  [[nodiscard]] static const ResourceAuthoritySnapshot* snapshot_for(
      PermissionResource resource,
      const EvaluationContext& context) {
    const auto iterator = std::find_if(
        context.resources.begin(), context.resources.end(),
        [resource](const ResourceAuthoritySnapshot& snapshot) {
          return snapshot.resource == resource;
        });
    return iterator == context.resources.end() ? nullptr : &*iterator;
  }

  [[nodiscard]] static std::optional<PermissionResolution> check_authority(
      PermissionResource resource,
      const AuthorityGate& gate,
      PermissionDecision denied_decision,
      std::string_view authority_name) {
    switch (gate.decision) {
      case AuthorityDecision::allow:
        return std::nullopt;
      case AuthorityDecision::not_required:
        if (!gate.required) {
          return std::nullopt;
        }
        return fail(resource, PermissionDecision::unavailable,
                    std::string{authority_name} + "-required-but-missing");
      case AuthorityDecision::deny:
        return fail(resource, denied_decision,
                    std::string{authority_name} + "-denied");
      case AuthorityDecision::unavailable:
        if (!gate.required) {
          return std::nullopt;
        }
        return fail(resource, PermissionDecision::unavailable,
                    std::string{authority_name} + "-unavailable");
      case AuthorityDecision::invalid:
        return fail(resource, PermissionDecision::error_fail_closed,
                    std::string{authority_name} + "-invalid");
    }
    return fail(resource, PermissionDecision::error_fail_closed,
                std::string{authority_name} + "-unknown");
  }

  [[nodiscard]] static PermissionResolution preflight_resource(
      const PermissionRequest& request,
      PermissionResource resource,
      const EvaluationContext& context) {
    (void)request;
    const auto* snapshot = snapshot_for(resource, context);
    if (snapshot == nullptr) {
      return fail(resource, PermissionDecision::unavailable,
                  "resource-authority-snapshot-missing");
    }

    if (const auto blocked =
            check_authority(resource, snapshot->goreecloud_policy,
                            PermissionDecision::blocked_policy,
                            "goreecloud-policy");
        blocked.has_value()) {
      return *blocked;
    }
    if (const auto blocked =
            check_authority(resource, snapshot->privacy_shield,
                            PermissionDecision::blocked_policy,
                            "privacy-shield");
        blocked.has_value()) {
      return *blocked;
    }
    if (const auto blocked =
            check_authority(resource, snapshot->wardveil_security,
                            PermissionDecision::blocked_security,
                            "wardveil-security");
        blocked.has_value()) {
      return *blocked;
    }

    switch (snapshot->host_os) {
      case HostOsPermissionState::granted:
        return PermissionResolution{
            .resource = resource,
            .decision = PermissionDecision::pending_user_decision,
            .engine_grant_allowed = false,
            .persistent_store_allowed = false,
            .reason = "authority-and-os-preflight-passed",
        };
      case HostOsPermissionState::denied_requestable:
      case HostOsPermissionState::denied_no_reprompt:
      case HostOsPermissionState::restricted:
        return fail(resource, PermissionDecision::blocked_os,
                    "host-os-not-granted");
      case HostOsPermissionState::unavailable:
        return fail(resource, PermissionDecision::unavailable,
                    "host-os-unavailable");
      case HostOsPermissionState::error:
        return fail(resource, PermissionDecision::error_fail_closed,
                    "host-os-error");
    }
    return fail(resource, PermissionDecision::error_fail_closed,
                "host-os-unknown");
  }

  [[nodiscard]] static bool is_persistent(UserPermissionDecision decision) {
    return decision == UserPermissionDecision::allow_persistent ||
           decision == UserPermissionDecision::deny_persistent;
  }

  [[nodiscard]] static bool is_deny(UserPermissionDecision decision) {
    return decision == UserPermissionDecision::deny_once ||
           decision == UserPermissionDecision::deny_session ||
           decision == UserPermissionDecision::deny_persistent;
  }

  [[nodiscard]] static PermissionDecision map_user_decision(
      UserPermissionDecision decision) {
    switch (decision) {
      case UserPermissionDecision::allow_once:
        return PermissionDecision::allow_once;
      case UserPermissionDecision::allow_session:
        return PermissionDecision::allow_session;
      case UserPermissionDecision::allow_persistent:
        return PermissionDecision::allow_persistent;
      case UserPermissionDecision::deny_once:
        return PermissionDecision::deny_once;
      case UserPermissionDecision::deny_session:
        return PermissionDecision::deny_session;
      case UserPermissionDecision::deny_persistent:
        return PermissionDecision::deny_persistent;
    }
    return PermissionDecision::error_fail_closed;
  }

  [[nodiscard]] static std::vector<PermissionResolution> resolutions_for_all(
      const PermissionRequest& request,
      const CommonFailure& failure) {
    std::vector<PermissionResolution> result;
    result.reserve(request.resources.size());
    for (const auto resource : request.resources) {
      result.push_back(fail(resource, failure.decision, failure.reason));
    }
    return result;
  }
};

}  // namespace goreecloud::browser
