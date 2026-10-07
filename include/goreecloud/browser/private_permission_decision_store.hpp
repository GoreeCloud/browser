#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "goreecloud/browser/permission_broker.hpp"
#include "goreecloud/browser/permission_decision_store.hpp"

namespace goreecloud::browser {

inline constexpr std::size_t kPrivatePermissionMaxContexts = 128;
inline constexpr std::size_t kPrivatePermissionMaxContextLifetimes = 4096;
inline constexpr std::size_t kPrivatePermissionMaxDecisions = 4096;
inline constexpr std::size_t kPrivatePermissionMaxDecisionsPerContext = 256;

struct PrivatePermissionContext {
  std::string profile_id;
  std::string privacy_context_id;
  PrivacyContext privacy_context{PrivacyContext::private_browsing};
  std::uint32_t contract_version{1};
};

struct EphemeralPermissionDecision {
  std::string profile_id;
  std::string privacy_context_id;
  PrivacyContext privacy_context{PrivacyContext::private_browsing};
  std::string origin;
  PermissionResource resource{PermissionResource::camera};
  PermissionDecision decision{PermissionDecision::deny_session};
  PermissionDecisionSource source{PermissionDecisionSource::user};
  std::int64_t created_at_millis{0};
  std::int64_t expires_at_millis{0};
  std::int64_t revoked_at_millis{0};
  std::uint32_t contract_version{1};
};

struct PrivatePermissionContextCloseResult {
  bool accepted{false};
  bool first_close{false};
  std::size_t decisions_destroyed{0};
  std::size_t requests_cancelled{0};
};

inline bool private_permission_context_type(PrivacyContext context) {
  return context == PrivacyContext::private_browsing ||
         context == PrivacyContext::isolated_private;
}

inline bool valid_private_permission_context(
    const PrivatePermissionContext& context) {
  return context.contract_version == 1 &&
         permission_text_safe(context.profile_id, 128) &&
         permission_text_safe(context.privacy_context_id, 128) &&
         private_permission_context_type(context.privacy_context);
}

inline bool ephemeral_permission_decision_value(PermissionDecision decision) {
  return decision == PermissionDecision::allow_session ||
         decision == PermissionDecision::deny_session;
}

inline bool valid_ephemeral_permission_decision(
    const EphemeralPermissionDecision& record) {
  if (record.contract_version != 1 ||
      !permission_text_safe(record.profile_id, 128) ||
      !permission_text_safe(record.privacy_context_id, 128) ||
      !private_permission_context_type(record.privacy_context) ||
      !canonical_web_origin(record.origin) ||
      permission_resource_name(record.resource).empty() ||
      !ephemeral_permission_decision_value(record.decision) ||
      record.source != PermissionDecisionSource::user ||
      record.created_at_millis < 0 || record.expires_at_millis < 0 ||
      record.revoked_at_millis < 0) {
    return false;
  }
  if (record.expires_at_millis != 0 &&
      record.expires_at_millis <= record.created_at_millis) {
    return false;
  }
  return record.revoked_at_millis == 0 ||
         record.revoked_at_millis >= record.created_at_millis;
}

inline std::string private_permission_context_key(
    std::string_view profile_id,
    std::string_view privacy_context_id) {
  std::string key;
  key.reserve(profile_id.size() + privacy_context_id.size() + 1U);
  key.append(profile_id);
  key.push_back('\n');
  key.append(privacy_context_id);
  return key;
}

inline bool same_ephemeral_permission_key(
    const EphemeralPermissionDecision& left,
    const EphemeralPermissionDecision& right) {
  return left.profile_id == right.profile_id &&
         left.privacy_context_id == right.privacy_context_id &&
         left.privacy_context == right.privacy_context &&
         left.origin == right.origin && left.resource == right.resource;
}

inline bool identical_ephemeral_permission_decision(
    const EphemeralPermissionDecision& left,
    const EphemeralPermissionDecision& right) {
  return same_ephemeral_permission_key(left, right) &&
         left.decision == right.decision && left.source == right.source &&
         left.created_at_millis == right.created_at_millis &&
         left.expires_at_millis == right.expires_at_millis &&
         left.revoked_at_millis == right.revoked_at_millis &&
         left.contract_version == right.contract_version;
}

class PrivatePermissionDecisionStore {
 public:
  [[nodiscard]] bool open_context(PrivatePermissionContext context) {
    if (!valid_private_permission_context(context)) {
      return false;
    }
    const auto key =
        private_permission_context_key(context.profile_id,
                                       context.privacy_context_id);
    if (closed_contexts_.contains(key)) {
      return false;
    }

    const auto existing = contexts_.find(key);
    if (existing != contexts_.end()) {
      return existing->second.profile_id == context.profile_id &&
             existing->second.privacy_context_id ==
                 context.privacy_context_id &&
             existing->second.privacy_context == context.privacy_context &&
             existing->second.contract_version == context.contract_version;
    }
    if (contexts_.size() >= kPrivatePermissionMaxContexts ||
        contexts_.size() + closed_contexts_.size() >=
            kPrivatePermissionMaxContextLifetimes) {
      return false;
    }
    contexts_.emplace(key, std::move(context));
    return true;
  }

  [[nodiscard]] bool upsert(EphemeralPermissionDecision record) {
    if (!valid_ephemeral_permission_decision(record)) {
      return false;
    }
    const auto context_key =
        private_permission_context_key(record.profile_id,
                                       record.privacy_context_id);
    if (closed_contexts_.contains(context_key)) {
      return false;
    }
    const auto context = contexts_.find(context_key);
    if (context == contexts_.end() ||
        context->second.privacy_context != record.privacy_context) {
      return false;
    }

    auto* existing = find_mutable(
        record.profile_id, record.privacy_context_id, record.privacy_context,
        record.origin, record.resource);
    if (existing != nullptr) {
      if (record.created_at_millis < existing->created_at_millis ||
          (existing->revoked_at_millis != 0 &&
           record.created_at_millis <= existing->revoked_at_millis)) {
        return false;
      }
      if (record.created_at_millis == existing->created_at_millis) {
        return identical_ephemeral_permission_decision(*existing, record);
      }
      *existing = std::move(record);
      return true;
    }

    if (decisions_.size() >= kPrivatePermissionMaxDecisions ||
        decision_count_for_context(context_key) >=
            kPrivatePermissionMaxDecisionsPerContext) {
      return false;
    }
    decisions_.push_back(std::move(record));
    return true;
  }

  [[nodiscard]] std::optional<EphemeralPermissionDecision> lookup(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context,
      std::string_view origin,
      PermissionResource resource,
      std::int64_t now_millis) const {
    if (now_millis < 0 ||
        !permission_text_safe(profile_id, 128) ||
        !permission_text_safe(privacy_context_id, 128) ||
        !private_permission_context_type(privacy_context) ||
        !canonical_web_origin(origin) ||
        permission_resource_name(resource).empty()) {
      return std::nullopt;
    }

    const auto context_key =
        private_permission_context_key(profile_id, privacy_context_id);
    if (closed_contexts_.contains(context_key)) {
      return std::nullopt;
    }
    const auto context = contexts_.find(context_key);
    if (context == contexts_.end() ||
        context->second.privacy_context != privacy_context) {
      return std::nullopt;
    }

    const auto* record =
        find(profile_id, privacy_context_id, privacy_context, origin, resource);
    if (record == nullptr || now_millis < record->created_at_millis ||
        (record->revoked_at_millis != 0 &&
         now_millis >= record->revoked_at_millis) ||
        (record->expires_at_millis != 0 &&
         now_millis >= record->expires_at_millis)) {
      return std::nullopt;
    }
    return *record;
  }

  [[nodiscard]] bool revoke(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context,
      std::string_view origin,
      PermissionResource resource,
      std::int64_t now_millis) {
    auto* record =
        find_mutable(profile_id, privacy_context_id, privacy_context, origin,
                     resource);
    if (record == nullptr || now_millis < record->created_at_millis) {
      return false;
    }
    if (record->revoked_at_millis != 0) {
      return now_millis >= record->revoked_at_millis;
    }
    record->revoked_at_millis = now_millis;
    return true;
  }

  [[nodiscard]] std::size_t reset_origin(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context,
      std::string_view origin,
      std::int64_t now_millis) {
    if (now_millis < 0 || !permission_text_safe(profile_id, 128) ||
        !permission_text_safe(privacy_context_id, 128) ||
        !private_permission_context_type(privacy_context) ||
        !canonical_web_origin(origin)) {
      return 0;
    }

    const auto context_key =
        private_permission_context_key(profile_id, privacy_context_id);
    if (closed_contexts_.contains(context_key)) {
      return 0;
    }

    std::size_t changed = 0;
    for (auto& record : decisions_) {
      if (record.profile_id != profile_id ||
          record.privacy_context_id != privacy_context_id ||
          record.privacy_context != privacy_context ||
          record.origin != origin ||
          now_millis < record.created_at_millis ||
          record.revoked_at_millis != 0) {
        continue;
      }
      record.revoked_at_millis = now_millis;
      ++changed;
    }
    return changed;
  }

  [[nodiscard]] PrivatePermissionContextCloseResult close_context(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context) {
    PrivatePermissionContextCloseResult result;
    if (!permission_text_safe(profile_id, 128) ||
        !permission_text_safe(privacy_context_id, 128) ||
        !private_permission_context_type(privacy_context)) {
      return result;
    }

    const auto key =
        private_permission_context_key(profile_id, privacy_context_id);
    const auto closed = closed_contexts_.find(key);
    if (closed != closed_contexts_.end()) {
      result.accepted = closed->second == privacy_context;
      return result;
    }

    const auto context = contexts_.find(key);
    if (context == contexts_.end() ||
        context->second.privacy_context != privacy_context) {
      return result;
    }

    const auto before = decisions_.size();
    decisions_.erase(
        std::remove_if(
            decisions_.begin(), decisions_.end(),
            [&](const EphemeralPermissionDecision& record) {
              return record.profile_id == profile_id &&
                     record.privacy_context_id == privacy_context_id &&
                     record.privacy_context == privacy_context;
            }),
        decisions_.end());

    contexts_.erase(context);
    closed_contexts_.emplace(key, privacy_context);
    result.accepted = true;
    result.first_close = true;
    result.decisions_destroyed = before - decisions_.size();
    return result;
  }

  [[nodiscard]] bool is_context_closed(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context) const {
    const auto key =
        private_permission_context_key(profile_id, privacy_context_id);
    const auto closed = closed_contexts_.find(key);
    return closed != closed_contexts_.end() &&
           closed->second == privacy_context;
  }

  [[nodiscard]] std::size_t decision_count() const noexcept {
    return decisions_.size();
  }

 private:
  std::unordered_map<std::string, PrivatePermissionContext> contexts_;
  std::unordered_map<std::string, PrivacyContext> closed_contexts_;
  std::vector<EphemeralPermissionDecision> decisions_;

  [[nodiscard]] std::size_t decision_count_for_context(
      std::string_view context_key) const {
    const auto separator = context_key.find('\n');
    if (separator == std::string_view::npos) {
      return 0;
    }
    const auto profile_id = context_key.substr(0, separator);
    const auto context_id = context_key.substr(separator + 1);
    return static_cast<std::size_t>(std::count_if(
        decisions_.begin(), decisions_.end(),
        [&](const EphemeralPermissionDecision& record) {
          return record.profile_id == profile_id &&
                 record.privacy_context_id == context_id;
        }));
  }

  [[nodiscard]] const EphemeralPermissionDecision* find(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context,
      std::string_view origin,
      PermissionResource resource) const {
    const auto iterator = std::find_if(
        decisions_.begin(), decisions_.end(),
        [&](const EphemeralPermissionDecision& record) {
          return record.profile_id == profile_id &&
                 record.privacy_context_id == privacy_context_id &&
                 record.privacy_context == privacy_context &&
                 record.origin == origin && record.resource == resource;
        });
    return iterator == decisions_.end() ? nullptr : &*iterator;
  }

  [[nodiscard]] EphemeralPermissionDecision* find_mutable(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context,
      std::string_view origin,
      PermissionResource resource) {
    const auto iterator = std::find_if(
        decisions_.begin(), decisions_.end(),
        [&](const EphemeralPermissionDecision& record) {
          return record.profile_id == profile_id &&
                 record.privacy_context_id == privacy_context_id &&
                 record.privacy_context == privacy_context &&
                 record.origin == origin && record.resource == resource;
        });
    return iterator == decisions_.end() ? nullptr : &*iterator;
  }
};

class PrivatePermissionLifecycleCoordinator {
 public:
  PrivatePermissionLifecycleCoordinator(PermissionBroker& broker,
                                        PrivatePermissionDecisionStore& store)
      : broker_(broker), store_(store) {}

  [[nodiscard]] PrivatePermissionContextCloseResult close_and_forget(
      std::string_view profile_id,
      std::string_view privacy_context_id,
      PrivacyContext privacy_context) {
    auto result =
        store_.close_context(profile_id, privacy_context_id, privacy_context);
    if (!result.accepted) {
      return result;
    }
    result.requests_cancelled =
        broker_.close_context(profile_id, privacy_context_id, privacy_context);
    return result;
  }

 private:
  PermissionBroker& broker_;
  PrivatePermissionDecisionStore& store_;
};

}  // namespace goreecloud::browser
