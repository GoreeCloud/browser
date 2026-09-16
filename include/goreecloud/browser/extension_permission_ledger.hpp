#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/native_extension.hpp"

namespace goreecloud::browser {

inline constexpr std::uint64_t kExtensionOneHourMillis =
    60ULL * 60ULL * 1000ULL;

// ExtensionPermissionLease is Browser-owned authorization state. It wraps a
// declarative permission grant with the lifecycle data required to make
// temporary grants fail closed rather than relying on caller-maintained booleans.
struct ExtensionPermissionLease {
  std::uint64_t id{0};
  ExtensionPermissionGrant grant;
  std::uint64_t issued_at_millis{0};
  std::uint64_t expires_at_millis{0};
  std::string tab_id;
  std::string browser_session_id;
  bool revoked{false};
  bool consumed{false};
};

struct ExtensionPermissionLeaseContext {
  ExtensionAuthorizationContext authorization;
  std::uint64_t now_millis{0};
  std::string_view tab_id;
  std::string_view browser_session_id;
};

inline bool valid_extension_permission_lease(
    const ExtensionPermissionLease& lease) {
  if (lease.id == 0 || lease.revoked || !lease.grant.active ||
      lease.grant.profile_id.empty()) {
    return false;
  }

  if (extension_permission_requires_website(lease.grant.permission) &&
      lease.grant.scope == ExtensionGrantScope::extension) {
    return false;
  }

  switch (lease.grant.scope) {
    case ExtensionGrantScope::extension:
    case ExtensionGrantScope::only_when_clicked:
    case ExtensionGrantScope::all_websites:
      break;
    case ExtensionGrantScope::current_website:
      if (lease.grant.websites.size() != 1 ||
          !valid_extension_site_declaration(lease.grant.websites.front())) {
        return false;
      }
      break;
    case ExtensionGrantScope::selected_websites:
      if (lease.grant.websites.empty() ||
          std::any_of(lease.grant.websites.begin(), lease.grant.websites.end(),
                      [](const std::string& site) {
                        return !valid_extension_site_declaration(site);
                      })) {
        return false;
      }
      break;
  }

  switch (lease.grant.lifetime) {
    case ExtensionGrantLifetime::once:
      return !lease.consumed && lease.issued_at_millis == 0 &&
             lease.expires_at_millis == 0;
    case ExtensionGrantLifetime::until_tab_closes:
      return !lease.tab_id.empty() && lease.issued_at_millis == 0 &&
             lease.expires_at_millis == 0;
    case ExtensionGrantLifetime::until_website_closes:
      return !lease.grant.websites.empty() &&
             std::all_of(lease.grant.websites.begin(),
                         lease.grant.websites.end(),
                         [](const std::string& site) {
                           return valid_extension_site_declaration(site);
                         }) &&
             lease.issued_at_millis == 0 && lease.expires_at_millis == 0;
    case ExtensionGrantLifetime::until_browser_closes:
      return !lease.browser_session_id.empty() && lease.issued_at_millis == 0 &&
             lease.expires_at_millis == 0;
    case ExtensionGrantLifetime::one_hour:
      return lease.issued_at_millis > 0 &&
             lease.issued_at_millis <=
                 std::numeric_limits<std::uint64_t>::max() -
                     kExtensionOneHourMillis &&
             lease.expires_at_millis ==
                 lease.issued_at_millis + kExtensionOneHourMillis;
    case ExtensionGrantLifetime::always:
      return lease.issued_at_millis == 0 && lease.expires_at_millis == 0;
  }
  return false;
}

class ExtensionPermissionLedger {
 public:
  [[nodiscard]] bool add(ExtensionPermissionLease lease) {
    if (!valid_extension_permission_lease(lease) ||
        std::any_of(leases_.begin(), leases_.end(),
                    [id = lease.id](const ExtensionPermissionLease& existing) {
                      return existing.id == id;
                    })) {
      return false;
    }
    leases_.push_back(std::move(lease));
    return true;
  }

  [[nodiscard]] bool revoke(std::uint64_t id) {
    const auto found = std::find_if(
        leases_.begin(), leases_.end(),
        [id](const ExtensionPermissionLease& lease) { return lease.id == id; });
    if (found == leases_.end() || found->revoked) {
      return false;
    }
    found->revoked = true;
    found->grant.active = false;
    return true;
  }

  [[nodiscard]] std::size_t revoke_profile(std::string_view profile_id) {
    std::size_t revoked = 0;
    for (auto& lease : leases_) {
      if (!lease.revoked && lease.grant.active &&
          lease.grant.profile_id == profile_id) {
        lease.revoked = true;
        lease.grant.active = false;
        ++revoked;
      }
    }
    return revoked;
  }

  [[nodiscard]] std::size_t close_tab(std::string_view tab_id) {
    return deactivate_matching([tab_id](const ExtensionPermissionLease& lease) {
      return lease.grant.lifetime == ExtensionGrantLifetime::until_tab_closes &&
             lease.tab_id == tab_id;
    });
  }

  [[nodiscard]] std::size_t close_website_scope(std::string_view profile_id,
                                                std::string_view website) {
    return deactivate_matching(
        [profile_id, website](const ExtensionPermissionLease& lease) {
          return lease.grant.lifetime ==
                     ExtensionGrantLifetime::until_website_closes &&
                 lease.grant.profile_id == profile_id &&
                 std::find(lease.grant.websites.begin(), lease.grant.websites.end(),
                           website) != lease.grant.websites.end();
        });
  }

  [[nodiscard]] std::size_t close_browser_session(
      std::string_view browser_session_id) {
    return deactivate_matching(
        [browser_session_id](const ExtensionPermissionLease& lease) {
          return lease.grant.lifetime ==
                     ExtensionGrantLifetime::until_browser_closes &&
                 lease.browser_session_id == browser_session_id;
        });
  }

  [[nodiscard]] std::size_t expire_at(std::uint64_t now_millis) {
    return deactivate_matching([now_millis](const ExtensionPermissionLease& lease) {
      return lease.grant.lifetime == ExtensionGrantLifetime::one_hour &&
             lease.expires_at_millis > 0 && now_millis >= lease.expires_at_millis;
    });
  }

  [[nodiscard]] bool authorize_and_consume(
      const ExtensionManifest& manifest,
      ExtensionPermission requested_permission,
      const ExtensionPermissionLeaseContext& context) {
    if (!validate_extension_manifest(manifest).accepted()) {
      return false;
    }

    for (auto& lease : leases_) {
      if (!lease.grant.active || lease.revoked || lease.consumed ||
          lease.grant.permission != requested_permission ||
          lease.grant.profile_id != context.authorization.profile_id) {
        continue;
      }

      auto authorization = context.authorization;
      switch (lease.grant.lifetime) {
        case ExtensionGrantLifetime::once:
          authorization.once_available = true;
          break;
        case ExtensionGrantLifetime::until_tab_closes:
          authorization.tab_active =
              authorization.tab_active && !context.tab_id.empty() &&
              lease.tab_id == context.tab_id;
          break;
        case ExtensionGrantLifetime::until_website_closes:
          authorization.website_active =
              authorization.website_active && !authorization.website.empty();
          break;
        case ExtensionGrantLifetime::until_browser_closes:
          authorization.browser_active =
              authorization.browser_active &&
              !context.browser_session_id.empty() &&
              lease.browser_session_id == context.browser_session_id;
          break;
        case ExtensionGrantLifetime::one_hour:
          authorization.one_hour_window_active =
              context.now_millis >= lease.issued_at_millis &&
              context.now_millis < lease.expires_at_millis;
          break;
        case ExtensionGrantLifetime::always:
          break;
      }

      const std::vector<ExtensionPermissionGrant> candidate{lease.grant};
      if (!authorize_extension_permission(manifest, candidate,
                                          requested_permission,
                                          authorization)) {
        continue;
      }

      if (lease.grant.lifetime == ExtensionGrantLifetime::once) {
        lease.consumed = true;
        lease.grant.active = false;
      }
      return true;
    }
    return false;
  }

  [[nodiscard]] const std::vector<ExtensionPermissionLease>& leases() const
      noexcept {
    return leases_;
  }

 private:
  template <typename Predicate>
  std::size_t deactivate_matching(Predicate predicate) {
    std::size_t deactivated = 0;
    for (auto& lease : leases_) {
      if (lease.grant.active && !lease.revoked && predicate(lease)) {
        lease.grant.active = false;
        ++deactivated;
      }
    }
    return deactivated;
  }

  std::vector<ExtensionPermissionLease> leases_;
};

}  // namespace goreecloud::browser
