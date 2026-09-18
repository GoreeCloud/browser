#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace goreecloud::browser {

inline constexpr int kNativeExtensionManifestVersion = 1;
inline constexpr int kNativeExtensionApiVersion = 1;
inline constexpr std::string_view kNativeExtensionManifestPath = "manifest";
inline constexpr std::string_view kNativeExtensionPackageSuffix = ".gcex";

enum class ExtensionPermission {
  read_current_page,
  modify_current_page,
  selected_websites,
  all_websites,
  read_tabs,
  create_tabs,
  read_history,
  modify_bookmarks,
  manage_downloads,
  clipboard,
  display_notifications,
  extension_storage,
  network_requests,
  context_menu_actions,
  browser_ui_components,
};

inline constexpr std::string_view extension_permission_name(
    ExtensionPermission permission) {
  switch (permission) {
    case ExtensionPermission::read_current_page:
      return "page.read";
    case ExtensionPermission::modify_current_page:
      return "page.modify";
    case ExtensionPermission::selected_websites:
      return "sites.selected";
    case ExtensionPermission::all_websites:
      return "sites.all";
    case ExtensionPermission::read_tabs:
      return "tabs.read";
    case ExtensionPermission::create_tabs:
      return "tabs.create";
    case ExtensionPermission::read_history:
      return "history.read";
    case ExtensionPermission::modify_bookmarks:
      return "bookmarks.modify";
    case ExtensionPermission::manage_downloads:
      return "downloads.manage";
    case ExtensionPermission::clipboard:
      return "clipboard.access";
    case ExtensionPermission::display_notifications:
      return "notifications.display";
    case ExtensionPermission::extension_storage:
      return "storage.extension";
    case ExtensionPermission::network_requests:
      return "network.request";
    case ExtensionPermission::context_menu_actions:
      return "context_menu.add";
    case ExtensionPermission::browser_ui_components:
      return "browser_ui.add";
  }
  return {};
}

inline constexpr std::string_view extension_permission_label(
    ExtensionPermission permission) {
  switch (permission) {
    case ExtensionPermission::read_current_page:
      return "Read the current page";
    case ExtensionPermission::modify_current_page:
      return "Modify the current page";
    case ExtensionPermission::selected_websites:
      return "Access selected websites";
    case ExtensionPermission::all_websites:
      return "Access all websites";
    case ExtensionPermission::read_tabs:
      return "Read open tabs";
    case ExtensionPermission::create_tabs:
      return "Create tabs";
    case ExtensionPermission::read_history:
      return "Read browsing history";
    case ExtensionPermission::modify_bookmarks:
      return "Modify bookmarks";
    case ExtensionPermission::manage_downloads:
      return "Manage downloads";
    case ExtensionPermission::clipboard:
      return "Access the clipboard";
    case ExtensionPermission::display_notifications:
      return "Display notifications";
    case ExtensionPermission::extension_storage:
      return "Store extension data";
    case ExtensionPermission::network_requests:
      return "Perform network requests";
    case ExtensionPermission::context_menu_actions:
      return "Add context-menu actions";
    case ExtensionPermission::browser_ui_components:
      return "Add browser interface components";
  }
  return {};
}

inline std::optional<ExtensionPermission> parse_extension_permission(
    std::string_view value) {
  constexpr ExtensionPermission permissions[] = {
      ExtensionPermission::read_current_page,
      ExtensionPermission::modify_current_page,
      ExtensionPermission::selected_websites,
      ExtensionPermission::all_websites,
      ExtensionPermission::read_tabs,
      ExtensionPermission::create_tabs,
      ExtensionPermission::read_history,
      ExtensionPermission::modify_bookmarks,
      ExtensionPermission::manage_downloads,
      ExtensionPermission::clipboard,
      ExtensionPermission::display_notifications,
      ExtensionPermission::extension_storage,
      ExtensionPermission::network_requests,
      ExtensionPermission::context_menu_actions,
      ExtensionPermission::browser_ui_components,
  };

  for (const auto permission : permissions) {
    if (extension_permission_name(permission) == value) {
      return permission;
    }
  }
  return std::nullopt;
}

enum class ExtensionTrustState {
  verified_signature,
  signed_package,
  local_development,
  unsigned_package,
};

enum class ExtensionValidationIssue {
  missing_id,
  invalid_id,
  missing_name,
  invalid_version,
  unsupported_manifest_version,
  unsupported_api_version,
  missing_entry_point,
  invalid_entry_point,
  duplicate_entry_point,
  unknown_permission,
  duplicate_permission,
  invalid_site_declaration,
  duplicate_site_declaration,
  invalid_package_suffix,
  empty_package,
  invalid_package_path,
  duplicate_package_entry,
  missing_manifest_entry,
  duplicate_manifest_entry,
  entry_point_missing_from_package,
};

struct ExtensionValidationResult {
  std::vector<ExtensionValidationIssue> issues;

  [[nodiscard]] bool accepted() const noexcept { return issues.empty(); }
};

struct ExtensionManifest {
  int manifest_version{kNativeExtensionManifestVersion};
  int api_version{kNativeExtensionApiVersion};
  std::string id;
  std::string name;
  std::string version;
  std::string description;
  std::string author;
  std::string homepage;
  std::string minimum_browser_version;
  std::vector<std::string> entry_points;
  std::vector<std::string> requested_permissions;
  std::vector<std::string> allowed_websites;
};

struct ExtensionPackageEntry {
  std::string path;
  std::uint64_t uncompressed_bytes{0};
};

struct ExtensionPackageInventory {
  std::string package_name;
  ExtensionTrustState trust_state{ExtensionTrustState::unsigned_package};
  std::vector<ExtensionPackageEntry> entries;
};

inline bool extension_string_has_control_or_space(std::string_view value) {
  return std::any_of(value.begin(), value.end(), [](unsigned char character) {
    return std::iscntrl(character) != 0 || std::isspace(character) != 0;
  });
}

inline bool valid_extension_id(std::string_view value) {
  if (value.size() < 3 || value.size() > 128) {
    return false;
  }
  if (!std::isalnum(static_cast<unsigned char>(value.front())) ||
      !std::isalnum(static_cast<unsigned char>(value.back()))) {
    return false;
  }
  for (const unsigned char character : value) {
    if (character >= 'A' && character <= 'Z') {
      return false;
    }
    if (!std::isalnum(character) && character != '.' && character != '-' &&
        character != '_') {
      return false;
    }
  }
  return value.find("..") == std::string_view::npos;
}

inline bool valid_extension_version(std::string_view value) {
  if (value.empty() || value.size() > 64 ||
      !std::isdigit(static_cast<unsigned char>(value.front()))) {
    return false;
  }
  return std::all_of(value.begin(), value.end(), [](unsigned char character) {
    return std::isalnum(character) != 0 || character == '.' ||
           character == '-' || character == '+';
  });
}

inline bool valid_extension_package_path(std::string_view path) {
  if (path.empty() || path.size() > 1024 || path.front() == '/' ||
      path.find('\\') != std::string_view::npos ||
      path.find(':') != std::string_view::npos ||
      extension_string_has_control_or_space(path)) {
    return false;
  }

  std::size_t start = 0;
  while (start < path.size()) {
    const auto separator = path.find('/', start);
    const auto end = separator == std::string_view::npos ? path.size() : separator;
    const auto segment = path.substr(start, end - start);
    if (segment.empty() || segment == "." || segment == "..") {
      return false;
    }
    if (separator == std::string_view::npos) {
      break;
    }
    start = separator + 1;
  }
  return true;
}

inline bool valid_extension_site_declaration(std::string_view site) {
  if (site.empty() || site.size() > 2048 ||
      extension_string_has_control_or_space(site)) {
    return false;
  }
  return site.starts_with("https://") || site.starts_with("http://");
}

inline void add_extension_issue_once(ExtensionValidationResult& result,
                                     ExtensionValidationIssue issue) {
  if (std::find(result.issues.begin(), result.issues.end(), issue) ==
      result.issues.end()) {
    result.issues.push_back(issue);
  }
}

inline ExtensionValidationResult validate_extension_manifest(
    const ExtensionManifest& manifest) {
  ExtensionValidationResult result;

  if (manifest.id.empty()) {
    add_extension_issue_once(result, ExtensionValidationIssue::missing_id);
  } else if (!valid_extension_id(manifest.id)) {
    add_extension_issue_once(result, ExtensionValidationIssue::invalid_id);
  }
  if (manifest.name.empty()) {
    add_extension_issue_once(result, ExtensionValidationIssue::missing_name);
  }
  if (!valid_extension_version(manifest.version)) {
    add_extension_issue_once(result, ExtensionValidationIssue::invalid_version);
  }
  if (manifest.manifest_version != kNativeExtensionManifestVersion) {
    add_extension_issue_once(result,
                             ExtensionValidationIssue::unsupported_manifest_version);
  }
  if (manifest.api_version != kNativeExtensionApiVersion) {
    add_extension_issue_once(result,
                             ExtensionValidationIssue::unsupported_api_version);
  }
  if (manifest.entry_points.empty()) {
    add_extension_issue_once(result, ExtensionValidationIssue::missing_entry_point);
  }

  std::vector<std::string_view> seen_entry_points;
  for (const auto& entry_point : manifest.entry_points) {
    if (!valid_extension_package_path(entry_point)) {
      add_extension_issue_once(result, ExtensionValidationIssue::invalid_entry_point);
    }
    if (std::find(seen_entry_points.begin(), seen_entry_points.end(), entry_point) !=
        seen_entry_points.end()) {
      add_extension_issue_once(result,
                               ExtensionValidationIssue::duplicate_entry_point);
    } else {
      seen_entry_points.emplace_back(entry_point);
    }
  }

  std::vector<std::string_view> seen_permissions;
  for (const auto& permission : manifest.requested_permissions) {
    if (!parse_extension_permission(permission).has_value()) {
      add_extension_issue_once(result, ExtensionValidationIssue::unknown_permission);
    }
    if (std::find(seen_permissions.begin(), seen_permissions.end(), permission) !=
        seen_permissions.end()) {
      add_extension_issue_once(result,
                               ExtensionValidationIssue::duplicate_permission);
    } else {
      seen_permissions.emplace_back(permission);
    }
  }

  std::vector<std::string_view> seen_sites;
  for (const auto& site : manifest.allowed_websites) {
    if (!valid_extension_site_declaration(site)) {
      add_extension_issue_once(result,
                               ExtensionValidationIssue::invalid_site_declaration);
    }
    if (std::find(seen_sites.begin(), seen_sites.end(), site) != seen_sites.end()) {
      add_extension_issue_once(
          result, ExtensionValidationIssue::duplicate_site_declaration);
    } else {
      seen_sites.emplace_back(site);
    }
  }

  return result;
}

inline ExtensionValidationResult validate_extension_package(
    const ExtensionPackageInventory& package,
    const ExtensionManifest& manifest) {
  auto result = validate_extension_manifest(manifest);

  if (!std::string_view{package.package_name}.ends_with(kNativeExtensionPackageSuffix)) {
    add_extension_issue_once(result,
                             ExtensionValidationIssue::invalid_package_suffix);
  }
  if (package.entries.empty()) {
    add_extension_issue_once(result, ExtensionValidationIssue::empty_package);
    add_extension_issue_once(result, ExtensionValidationIssue::missing_manifest_entry);
    return result;
  }

  std::vector<std::string_view> seen_paths;
  std::size_t manifest_entries = 0;
  for (const auto& entry : package.entries) {
    if (!valid_extension_package_path(entry.path)) {
      add_extension_issue_once(result,
                               ExtensionValidationIssue::invalid_package_path);
    }
    if (std::find(seen_paths.begin(), seen_paths.end(), entry.path) !=
        seen_paths.end()) {
      add_extension_issue_once(result,
                               ExtensionValidationIssue::duplicate_package_entry);
    } else {
      seen_paths.emplace_back(entry.path);
    }
    if (entry.path == kNativeExtensionManifestPath) {
      ++manifest_entries;
    }
  }

  if (manifest_entries == 0) {
    add_extension_issue_once(result, ExtensionValidationIssue::missing_manifest_entry);
  } else if (manifest_entries > 1) {
    add_extension_issue_once(result,
                             ExtensionValidationIssue::duplicate_manifest_entry);
  }

  for (const auto& entry_point : manifest.entry_points) {
    const auto found = std::any_of(
        package.entries.begin(), package.entries.end(),
        [&entry_point](const ExtensionPackageEntry& entry) {
          return entry.path == entry_point;
        });
    if (!found) {
      add_extension_issue_once(
          result, ExtensionValidationIssue::entry_point_missing_from_package);
    }
  }

  return result;
}

inline bool extension_manifest_declares_permission(
    const ExtensionManifest& manifest, ExtensionPermission permission) {
  const auto name = extension_permission_name(permission);
  return std::any_of(manifest.requested_permissions.begin(),
                     manifest.requested_permissions.end(),
                     [name](const std::string& declared) { return declared == name; });
}

inline bool extension_permission_requires_website(
    ExtensionPermission permission) {
  return permission == ExtensionPermission::read_current_page ||
         permission == ExtensionPermission::modify_current_page ||
         permission == ExtensionPermission::selected_websites ||
         permission == ExtensionPermission::all_websites;
}

inline bool extension_manifest_allows_website(
    const ExtensionManifest& manifest, std::string_view website) {
  if (website.empty()) {
    return false;
  }
  if (extension_manifest_declares_permission(
          manifest, ExtensionPermission::all_websites)) {
    return true;
  }
  return std::find(manifest.allowed_websites.begin(),
                   manifest.allowed_websites.end(), website) !=
         manifest.allowed_websites.end();
}

enum class ExtensionGrantLifetime {
  once,
  until_tab_closes,
  until_website_closes,
  until_browser_closes,
  one_hour,
  always,
};

enum class ExtensionGrantScope {
  extension,
  only_when_clicked,
  current_website,
  selected_websites,
  all_websites,
};

struct ExtensionPermissionGrant {
  ExtensionPermission permission{ExtensionPermission::read_current_page};
  std::string profile_id;
  ExtensionGrantScope scope{ExtensionGrantScope::extension};
  ExtensionGrantLifetime lifetime{ExtensionGrantLifetime::once};
  std::vector<std::string> websites;
  bool private_browsing_allowed{false};
  bool active{false};
};

struct ExtensionAuthorizationContext {
  std::string_view profile_id;
  std::string_view website;
  bool private_browsing{false};
  bool user_activation{false};
  bool once_available{false};
  bool tab_active{false};
  bool website_active{false};
  bool browser_active{false};
  bool one_hour_window_active{false};
};

inline bool extension_grant_lifetime_active(
    ExtensionGrantLifetime lifetime,
    const ExtensionAuthorizationContext& context) {
  switch (lifetime) {
    case ExtensionGrantLifetime::once:
      return context.once_available;
    case ExtensionGrantLifetime::until_tab_closes:
      return context.tab_active;
    case ExtensionGrantLifetime::until_website_closes:
      return context.website_active;
    case ExtensionGrantLifetime::until_browser_closes:
      return context.browser_active;
    case ExtensionGrantLifetime::one_hour:
      return context.one_hour_window_active;
    case ExtensionGrantLifetime::always:
      return true;
  }
  return false;
}

inline bool extension_grant_scope_authorizes(
    const ExtensionPermissionGrant& grant,
    const ExtensionAuthorizationContext& context) {
  switch (grant.scope) {
    case ExtensionGrantScope::extension:
      return true;
    case ExtensionGrantScope::only_when_clicked:
      return context.user_activation;
    case ExtensionGrantScope::current_website:
      return grant.websites.size() == 1 && !context.website.empty() &&
             grant.websites.front() == context.website;
    case ExtensionGrantScope::selected_websites:
      return !context.website.empty() &&
             std::find(grant.websites.begin(), grant.websites.end(), context.website) !=
                 grant.websites.end();
    case ExtensionGrantScope::all_websites:
      return !context.website.empty();
  }
  return false;
}

inline bool authorize_extension_permission(
    const ExtensionManifest& manifest,
    const std::vector<ExtensionPermissionGrant>& grants,
    ExtensionPermission requested_permission,
    const ExtensionAuthorizationContext& context) {
  if (!validate_extension_manifest(manifest).accepted() ||
      !extension_manifest_declares_permission(manifest, requested_permission) ||
      context.profile_id.empty()) {
    return false;
  }

  const bool website_required =
      extension_permission_requires_website(requested_permission);
  if (website_required && !extension_manifest_allows_website(manifest, context.website)) {
    return false;
  }

  return std::any_of(
      grants.begin(), grants.end(),
      [&](const ExtensionPermissionGrant& grant) {
        if (website_required && grant.scope == ExtensionGrantScope::extension) {
          return false;
        }
        return grant.active && grant.permission == requested_permission &&
               grant.profile_id == context.profile_id &&
               (!context.private_browsing || grant.private_browsing_allowed) &&
               extension_grant_lifetime_active(grant.lifetime, context) &&
               extension_grant_scope_authorizes(grant, context);
      });
}

}  // namespace goreecloud::browser
