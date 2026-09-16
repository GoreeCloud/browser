#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/extension_permission_ledger.hpp"
#include "goreecloud/browser/native_extension.hpp"

namespace goreecloud::browser {

inline constexpr std::uint64_t kExtensionRuntimeCapabilityMaxMillis = 5000ULL;

// This policy is a Browser-owned contract for a future platform process launcher.
// It does not claim that an OS sandbox or extension process exists yet.
struct ExtensionRuntimeProcessPolicy {
  bool dedicated_process_required{true};
  bool broker_only_privileged_api{true};
  bool direct_browser_internal_access{false};
  bool direct_other_extension_access{false};
  bool direct_goreecloud_service_access{false};
  bool ambient_operating_system_authority{false};
};

inline constexpr ExtensionRuntimeProcessPolicy
extension_runtime_process_policy() noexcept {
  return {};
}

enum class ExtensionRuntimeState {
  running,
  suspended,
  terminated,
};

struct ExtensionRuntimeIdentity {
  std::string extension_id;
  std::string profile_id;
  std::string runtime_instance_id;
  bool private_browsing{false};
};

struct ExtensionRuntimeLaunchDecision {
  bool browser_launch_authorized{false};
  bool private_browsing_authorized{false};
};

struct ExtensionRuntimeSession {
  ExtensionRuntimeIdentity identity;
  ExtensionTrustState package_trust_state{ExtensionTrustState::unsigned_package};
  ExtensionRuntimeState state{ExtensionRuntimeState::running};
};

struct ExtensionRuntimeCapabilityRequest {
  std::string_view extension_id;
  std::string_view profile_id;
  std::string_view runtime_instance_id;
  ExtensionPermission permission{ExtensionPermission::read_current_page};
  std::string_view website;
  std::string_view tab_id;
  std::string_view browser_session_id;
  bool private_browsing{false};
  bool user_activation{false};
  bool tab_active{false};
  bool website_active{false};
  bool browser_active{false};
  std::uint64_t now_millis{0};
};

struct ExtensionRuntimeCapabilityToken {
  std::uint64_t id{0};
  std::string extension_id;
  std::string profile_id;
  std::string runtime_instance_id;
  ExtensionPermission permission{ExtensionPermission::read_current_page};
  std::string website;
  std::string tab_id;
  std::string browser_session_id;
  bool private_browsing{false};
  std::uint64_t issued_at_millis{0};
  std::uint64_t expires_at_millis{0};
  bool revoked{false};
  bool consumed{false};
};

struct ExtensionRuntimeDispatchContext {
  std::string_view extension_id;
  std::string_view profile_id;
  std::string_view runtime_instance_id;
  ExtensionPermission permission{ExtensionPermission::read_current_page};
  std::string_view website;
  std::string_view tab_id;
  std::string_view browser_session_id;
  bool private_browsing{false};
  bool tab_active{false};
  bool website_active{false};
  bool browser_active{false};
  std::uint64_t now_millis{0};
};

inline bool valid_extension_runtime_identifier(std::string_view value) {
  return !value.empty() && value.size() <= 256 &&
         !extension_string_has_control_or_space(value);
}

class ExtensionRuntimeAuthority {
 public:
  [[nodiscard]] bool register_runtime(
      const ExtensionManifest& manifest,
      ExtensionTrustState package_trust_state,
      ExtensionRuntimeIdentity identity,
      const ExtensionRuntimeLaunchDecision& decision) {
    // Package trust is intentionally not an execution-authority shortcut.
    if (!decision.browser_launch_authorized ||
        !validate_extension_manifest(manifest).accepted() ||
        manifest.id != identity.extension_id ||
        !valid_extension_runtime_identifier(identity.extension_id) ||
        !valid_extension_runtime_identifier(identity.profile_id) ||
        !valid_extension_runtime_identifier(identity.runtime_instance_id) ||
        (identity.private_browsing && !decision.private_browsing_authorized) ||
        find_session(identity.runtime_instance_id) != sessions_.end()) {
      return false;
    }

    sessions_.push_back(ExtensionRuntimeSession{
        .identity = std::move(identity),
        .package_trust_state = package_trust_state,
        .state = ExtensionRuntimeState::running,
    });
    return true;
  }

  [[nodiscard]] bool suspend(std::string_view runtime_instance_id) {
    auto session = find_session(runtime_instance_id);
    if (session == sessions_.end() ||
        session->state != ExtensionRuntimeState::running) {
      return false;
    }
    session->state = ExtensionRuntimeState::suspended;
    revoke_runtime_tokens(runtime_instance_id);
    return true;
  }

  [[nodiscard]] bool resume(std::string_view runtime_instance_id,
                            bool browser_resume_authorized) {
    auto session = find_session(runtime_instance_id);
    if (!browser_resume_authorized || session == sessions_.end() ||
        session->state != ExtensionRuntimeState::suspended) {
      return false;
    }
    session->state = ExtensionRuntimeState::running;
    return true;
  }

  [[nodiscard]] bool terminate(std::string_view runtime_instance_id) {
    auto session = find_session(runtime_instance_id);
    if (session == sessions_.end() ||
        session->state == ExtensionRuntimeState::terminated) {
      return false;
    }
    session->state = ExtensionRuntimeState::terminated;
    revoke_runtime_tokens(runtime_instance_id);
    return true;
  }

  [[nodiscard]] std::optional<ExtensionRuntimeCapabilityToken>
  issue_capability(const ExtensionManifest& manifest,
                   ExtensionPermissionLedger& permission_ledger,
                   const ExtensionRuntimeCapabilityRequest& request) {
    auto session = find_session(request.runtime_instance_id);
    if (session == sessions_.end() ||
        session->state != ExtensionRuntimeState::running ||
        session->identity.extension_id != request.extension_id ||
        session->identity.profile_id != request.profile_id ||
        session->identity.private_browsing != request.private_browsing ||
        manifest.id != request.extension_id ||
        !extension_manifest_declares_permission(manifest, request.permission)) {
      return std::nullopt;
    }

    ExtensionPermissionLeaseContext lease_context;
    lease_context.authorization.profile_id = request.profile_id;
    lease_context.authorization.website = request.website;
    lease_context.authorization.private_browsing = request.private_browsing;
    lease_context.authorization.user_activation = request.user_activation;
    lease_context.authorization.tab_active = request.tab_active;
    lease_context.authorization.website_active = request.website_active;
    lease_context.authorization.browser_active = request.browser_active;
    lease_context.now_millis = request.now_millis;
    lease_context.tab_id = request.tab_id;
    lease_context.browser_session_id = request.browser_session_id;

    if (!permission_ledger.authorize_and_consume(
            manifest, request.permission, lease_context)) {
      return std::nullopt;
    }

    if (next_token_id_ == 0 ||
        request.now_millis >
            std::numeric_limits<std::uint64_t>::max() -
                kExtensionRuntimeCapabilityMaxMillis) {
      return std::nullopt;
    }

    ExtensionRuntimeCapabilityToken token{
        .id = next_token_id_++,
        .extension_id = std::string(request.extension_id),
        .profile_id = std::string(request.profile_id),
        .runtime_instance_id = std::string(request.runtime_instance_id),
        .permission = request.permission,
        .website = std::string(request.website),
        .tab_id = std::string(request.tab_id),
        .browser_session_id = std::string(request.browser_session_id),
        .private_browsing = request.private_browsing,
        .issued_at_millis = request.now_millis,
        .expires_at_millis =
            request.now_millis + kExtensionRuntimeCapabilityMaxMillis,
    };
    tokens_.push_back(token);
    return token;
  }

  [[nodiscard]] bool authorize_and_consume(
      std::uint64_t token_id,
      const ExtensionRuntimeDispatchContext& context) {
    auto token = find_token(token_id);
    if (token == tokens_.end() || token->revoked || token->consumed ||
        token->id == 0 || context.now_millis < token->issued_at_millis ||
        context.now_millis >= token->expires_at_millis ||
        token->extension_id != context.extension_id ||
        token->profile_id != context.profile_id ||
        token->runtime_instance_id != context.runtime_instance_id ||
        token->permission != context.permission ||
        token->website != context.website || token->tab_id != context.tab_id ||
        token->browser_session_id != context.browser_session_id ||
        token->private_browsing != context.private_browsing) {
      return false;
    }

    auto session = find_session(context.runtime_instance_id);
    if (session == sessions_.end() ||
        session->state != ExtensionRuntimeState::running ||
        session->identity.extension_id != context.extension_id ||
        session->identity.profile_id != context.profile_id ||
        session->identity.private_browsing != context.private_browsing) {
      return false;
    }

    if (!context.browser_active ||
        (!context.tab_id.empty() && !context.tab_active) ||
        (!context.website.empty() && !context.website_active)) {
      return false;
    }

    token->consumed = true;
    return true;
  }

  [[nodiscard]] const ExtensionRuntimeSession* session(
      std::string_view runtime_instance_id) const noexcept {
    const auto found = std::find_if(
        sessions_.begin(), sessions_.end(),
        [runtime_instance_id](const ExtensionRuntimeSession& candidate) {
          return candidate.identity.runtime_instance_id == runtime_instance_id;
        });
    return found == sessions_.end() ? nullptr : &*found;
  }

 private:
  auto find_session(std::string_view runtime_instance_id) {
    return std::find_if(
        sessions_.begin(), sessions_.end(),
        [runtime_instance_id](const ExtensionRuntimeSession& candidate) {
          return candidate.identity.runtime_instance_id == runtime_instance_id;
        });
  }

  auto find_token(std::uint64_t token_id) {
    return std::find_if(tokens_.begin(), tokens_.end(),
                        [token_id](const ExtensionRuntimeCapabilityToken& token) {
                          return token.id == token_id;
                        });
  }

  void revoke_runtime_tokens(std::string_view runtime_instance_id) {
    for (auto& token : tokens_) {
      if (!token.revoked && !token.consumed &&
          token.runtime_instance_id == runtime_instance_id) {
        token.revoked = true;
      }
    }
  }

  std::vector<ExtensionRuntimeSession> sessions_;
  std::vector<ExtensionRuntimeCapabilityToken> tokens_;
  std::uint64_t next_token_id_{1};
};

}  // namespace goreecloud::browser
