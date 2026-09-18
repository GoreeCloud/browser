#include <algorithm>
#include <cassert>
#include <string_view>
#include <vector>

#include "goreecloud/browser/extensions.hpp"

using goreecloud::browser::ExtensionAuthorizationContext;
using goreecloud::browser::ExtensionGrantLifetime;
using goreecloud::browser::ExtensionGrantScope;
using goreecloud::browser::ExtensionManifest;
using goreecloud::browser::ExtensionPackageEntry;
using goreecloud::browser::ExtensionPackageInventory;
using goreecloud::browser::ExtensionPermission;
using goreecloud::browser::ExtensionPermissionGrant;
using goreecloud::browser::ExtensionPermissionLease;
using goreecloud::browser::ExtensionPermissionLeaseContext;
using goreecloud::browser::ExtensionPermissionLedger;
using goreecloud::browser::ExtensionValidationIssue;

namespace {

bool has_issue(const goreecloud::browser::ExtensionValidationResult& result,
               ExtensionValidationIssue issue) {
  return std::find(result.issues.begin(), result.issues.end(), issue) !=
         result.issues.end();
}

ExtensionManifest sample_manifest() {
  return ExtensionManifest{
      .id = "org.example.reader-helper",
      .name = "Reader Helper",
      .version = "1.0.0",
      .description = "Test extension",
      .author = "Example",
      .homepage = "https://example.org",
      .minimum_browser_version = "0.1.0",
      .entry_points = {"scripts/background.js", "pages/panel.html"},
      .requested_permissions = {"page.read", "notifications.display"},
      .allowed_websites = {"https://example.org"},
  };
}

ExtensionPackageInventory sample_package() {
  return ExtensionPackageInventory{
      .package_name = "reader-helper.gcex",
      .entries = {
          ExtensionPackageEntry{.path = "manifest", .uncompressed_bytes = 512},
          ExtensionPackageEntry{.path = "scripts/background.js", .uncompressed_bytes = 128},
          ExtensionPackageEntry{.path = "pages/panel.html", .uncompressed_bytes = 256},
      },
  };
}

}  // namespace

int main() {
  const goreecloud::browser::ExtensionPolicy disabled_policy{};
  assert(disabled_policy.support == goreecloud::browser::ExtensionSupportState::disabled);
  assert(!disabled_policy.external_compatibility_layer_exposed);
  assert(!disabled_policy.centralized_store_exposed);
  assert(!disabled_policy.private_browsing_default_enabled);
  assert(!goreecloud::browser::extension_permission_label(
              ExtensionPermission::read_current_page).empty());

  auto manifest = sample_manifest();
  auto package = sample_package();
  assert(goreecloud::browser::validate_extension_manifest(manifest).accepted());
  assert(goreecloud::browser::validate_extension_package(package, manifest).accepted());

  auto unknown_permission = manifest;
  unknown_permission.requested_permissions.push_back("browser.unrestricted");
  const auto unknown_result =
      goreecloud::browser::validate_extension_manifest(unknown_permission);
  assert(!unknown_result.accepted());
  assert(has_issue(unknown_result, ExtensionValidationIssue::unknown_permission));

  auto traversing_package = package;
  traversing_package.entries.push_back(
      ExtensionPackageEntry{.path = "../escape.js", .uncompressed_bytes = 32});
  const auto traversal_result =
      goreecloud::browser::validate_extension_package(traversing_package, manifest);
  assert(!traversal_result.accepted());
  assert(has_issue(traversal_result, ExtensionValidationIssue::invalid_package_path));

  auto wrong_suffix = package;
  wrong_suffix.package_name = "reader-helper.zip";
  assert(has_issue(goreecloud::browser::validate_extension_package(wrong_suffix, manifest),
                   ExtensionValidationIssue::invalid_package_suffix));

  auto missing_entry_point = package;
  missing_entry_point.entries.pop_back();
  assert(has_issue(
      goreecloud::browser::validate_extension_package(missing_entry_point, manifest),
      ExtensionValidationIssue::entry_point_missing_from_package));

  std::vector<ExtensionPermissionGrant> grants{
      ExtensionPermissionGrant{
          .permission = ExtensionPermission::read_current_page,
          .profile_id = "profile-personal",
          .scope = ExtensionGrantScope::selected_websites,
          .lifetime = ExtensionGrantLifetime::one_hour,
          .websites = {"https://example.org"},
          .private_browsing_allowed = false,
          .active = true,
      },
  };

  ExtensionAuthorizationContext context{
      .profile_id = "profile-personal",
      .website = "https://example.org",
      .one_hour_window_active = true,
  };
  assert(goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::read_current_page, context));

  auto other_profile = context;
  other_profile.profile_id = "profile-work";
  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::read_current_page, other_profile));

  auto private_context = context;
  private_context.private_browsing = true;
  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::read_current_page, private_context));

  grants.front().private_browsing_allowed = true;
  assert(goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::read_current_page, private_context));

  auto expired_context = context;
  expired_context.one_hour_window_active = false;
  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::read_current_page, expired_context));

  auto wrong_site = context;
  wrong_site.website = "https://other.example";
  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::read_current_page, wrong_site));

  auto broad_grant = grants;
  broad_grant.front().scope = ExtensionGrantScope::all_websites;
  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, broad_grant, ExtensionPermission::read_current_page, wrong_site));

  auto undeclared_scope = grants;
  undeclared_scope.front().scope = ExtensionGrantScope::extension;
  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, undeclared_scope, ExtensionPermission::read_current_page, context));

  assert(!goreecloud::browser::authorize_extension_permission(
      manifest, grants, ExtensionPermission::modify_current_page, context));

  ExtensionPermissionLedger ledger;
  ExtensionPermissionLease once_lease{
      .id = 1,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = "profile-personal",
          .scope = ExtensionGrantScope::extension,
          .lifetime = ExtensionGrantLifetime::once,
          .websites = {},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = {},
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
  assert(ledger.add(once_lease));
  assert(!ledger.add(once_lease));

  ExtensionPermissionLeaseContext once_context{
      .authorization = ExtensionAuthorizationContext{
          .profile_id = "profile-personal",
          .website = {},
          .private_browsing = false,
          .user_activation = false,
          .once_available = false,
          .tab_active = false,
          .website_active = false,
          .browser_active = false,
          .one_hour_window_active = false,
      },
      .now_millis = 0,
      .tab_id = {},
      .browser_session_id = {},
  };
  assert(ledger.authorize_and_consume(
      manifest, ExtensionPermission::display_notifications, once_context));
  assert(!ledger.authorize_and_consume(
      manifest, ExtensionPermission::display_notifications, once_context));

  ExtensionPermissionLease timed_lease{
      .id = 2,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::read_current_page,
          .profile_id = "profile-personal",
          .scope = ExtensionGrantScope::selected_websites,
          .lifetime = ExtensionGrantLifetime::one_hour,
          .websites = {"https://example.org"},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 1000,
      .expires_at_millis = 1000 + goreecloud::browser::kExtensionOneHourMillis,
      .tab_id = {},
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
  assert(ledger.add(timed_lease));

  auto malformed_timed_lease = timed_lease;
  malformed_timed_lease.id = 3;
  malformed_timed_lease.expires_at_millis = timed_lease.issued_at_millis + 1;
  assert(!ledger.add(malformed_timed_lease));

  ExtensionPermissionLeaseContext timed_context{
      .authorization = ExtensionAuthorizationContext{
          .profile_id = "profile-personal",
          .website = "https://example.org",
          .private_browsing = false,
          .user_activation = false,
          .once_available = false,
          .tab_active = false,
          .website_active = false,
          .browser_active = false,
          .one_hour_window_active = false,
      },
      .now_millis = 2000,
      .tab_id = {},
      .browser_session_id = {},
  };
  assert(ledger.authorize_and_consume(
      manifest, ExtensionPermission::read_current_page, timed_context));

  auto timed_private_context = timed_context;
  timed_private_context.authorization.private_browsing = true;
  assert(!ledger.authorize_and_consume(
      manifest, ExtensionPermission::read_current_page, timed_private_context));

  timed_context.now_millis = timed_lease.expires_at_millis;
  assert(!ledger.authorize_and_consume(
      manifest, ExtensionPermission::read_current_page, timed_context));
  assert(ledger.expire_at(timed_lease.expires_at_millis) == 1);

  ExtensionPermissionLease tab_lease{
      .id = 4,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = "profile-personal",
          .scope = ExtensionGrantScope::extension,
          .lifetime = ExtensionGrantLifetime::until_tab_closes,
          .websites = {},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = "tab-1",
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
  assert(ledger.add(tab_lease));
  ExtensionPermissionLeaseContext tab_context{
      .authorization = ExtensionAuthorizationContext{
          .profile_id = "profile-personal",
          .website = {},
          .private_browsing = false,
          .user_activation = false,
          .once_available = false,
          .tab_active = true,
          .website_active = false,
          .browser_active = false,
          .one_hour_window_active = false,
      },
      .now_millis = 0,
      .tab_id = "tab-1",
      .browser_session_id = {},
  };
  assert(ledger.authorize_and_consume(
      manifest, ExtensionPermission::display_notifications, tab_context));
  tab_context.tab_id = "tab-2";
  assert(!ledger.authorize_and_consume(
      manifest, ExtensionPermission::display_notifications, tab_context));
  assert(ledger.close_tab("tab-1") == 1);

  ExtensionPermissionLease website_lease{
      .id = 5,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::read_current_page,
          .profile_id = "profile-personal",
          .scope = ExtensionGrantScope::selected_websites,
          .lifetime = ExtensionGrantLifetime::until_website_closes,
          .websites = {"https://example.org"},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = {},
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
  assert(ledger.add(website_lease));
  ExtensionPermissionLeaseContext website_context{
      .authorization = ExtensionAuthorizationContext{
          .profile_id = "profile-personal",
          .website = "https://example.org",
          .private_browsing = false,
          .user_activation = false,
          .once_available = false,
          .tab_active = false,
          .website_active = true,
          .browser_active = false,
          .one_hour_window_active = false,
      },
      .now_millis = 0,
      .tab_id = {},
      .browser_session_id = {},
  };
  assert(ledger.authorize_and_consume(
      manifest, ExtensionPermission::read_current_page, website_context));
  assert(ledger.close_website_scope("profile-personal", "https://example.org") ==
         1);
  assert(!ledger.authorize_and_consume(
      manifest, ExtensionPermission::read_current_page, website_context));

  ExtensionPermissionLease browser_lease{
      .id = 6,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = "profile-personal",
          .scope = ExtensionGrantScope::extension,
          .lifetime = ExtensionGrantLifetime::until_browser_closes,
          .websites = {},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = {},
      .browser_session_id = "browser-session-1",
      .revoked = false,
      .consumed = false,
  };
  assert(ledger.add(browser_lease));
  ExtensionPermissionLeaseContext browser_context{
      .authorization = ExtensionAuthorizationContext{
          .profile_id = "profile-personal",
          .website = {},
          .private_browsing = false,
          .user_activation = false,
          .once_available = false,
          .tab_active = false,
          .website_active = false,
          .browser_active = true,
          .one_hour_window_active = false,
      },
      .now_millis = 0,
      .tab_id = {},
      .browser_session_id = "browser-session-1",
  };
  assert(ledger.authorize_and_consume(
      manifest, ExtensionPermission::display_notifications, browser_context));
  browser_context.browser_session_id = "browser-session-2";
  assert(!ledger.authorize_and_consume(
      manifest, ExtensionPermission::display_notifications, browser_context));
  assert(ledger.close_browser_session("browser-session-1") == 1);

  ExtensionPermissionLease revocable_lease{
      .id = 7,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = "profile-work",
          .scope = ExtensionGrantScope::extension,
          .lifetime = ExtensionGrantLifetime::always,
          .websites = {},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = {},
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
  assert(ledger.add(revocable_lease));
  assert(ledger.revoke(7));
  assert(!ledger.revoke(7));

  ExtensionPermissionLease profile_lease_a = revocable_lease;
  profile_lease_a.id = 8;
  profile_lease_a.grant.active = true;
  profile_lease_a.revoked = false;
  ExtensionPermissionLease profile_lease_b = profile_lease_a;
  profile_lease_b.id = 9;
  assert(ledger.add(profile_lease_a));
  assert(ledger.add(profile_lease_b));
  assert(ledger.revoke_profile("profile-work") == 2);

  return 0;
}
