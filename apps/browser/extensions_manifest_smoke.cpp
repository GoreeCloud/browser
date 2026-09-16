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

  return 0;
}
