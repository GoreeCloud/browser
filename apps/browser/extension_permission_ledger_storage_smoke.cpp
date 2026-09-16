#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "goreecloud/browser/extension_permission_ledger_storage.hpp"

using goreecloud::browser::ExtensionGrantLifetime;
using goreecloud::browser::ExtensionGrantScope;
using goreecloud::browser::ExtensionPermission;
using goreecloud::browser::ExtensionPermissionGrant;
using goreecloud::browser::ExtensionPermissionLease;
using goreecloud::browser::ExtensionPermissionLedger;
using goreecloud::browser::ExtensionPermissionLedgerStorageIssue;

namespace {

bool has_issue(
    const goreecloud::browser::ExtensionPermissionLedgerRestoreResult& result,
    ExtensionPermissionLedgerStorageIssue issue) {
  return std::find(result.issues.begin(), result.issues.end(), issue) !=
         result.issues.end();
}

bool has_lease_id(const ExtensionPermissionLedger& ledger, std::uint64_t id) {
  return std::any_of(
      ledger.leases().begin(), ledger.leases().end(),
      [id](const ExtensionPermissionLease& lease) { return lease.id == id; });
}

ExtensionPermissionLease always_lease(std::uint64_t id,
                                      std::string_view profile_id) {
  return ExtensionPermissionLease{
      .id = id,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = std::string(profile_id),
          .scope = ExtensionGrantScope::extension,
          .lifetime = ExtensionGrantLifetime::always,
          .websites = {},
          .private_browsing_allowed = true,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = {},
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
}

ExtensionPermissionLease one_hour_lease(std::uint64_t id,
                                        std::string_view profile_id,
                                        std::uint64_t issued_at_millis) {
  return ExtensionPermissionLease{
      .id = id,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::read_current_page,
          .profile_id = std::string(profile_id),
          .scope = ExtensionGrantScope::selected_websites,
          .lifetime = ExtensionGrantLifetime::one_hour,
          .websites = {"https://example.org"},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = issued_at_millis,
      .expires_at_millis =
          issued_at_millis + goreecloud::browser::kExtensionOneHourMillis,
      .tab_id = {},
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
}

ExtensionPermissionLease once_lease(std::uint64_t id,
                                    std::string_view profile_id) {
  return ExtensionPermissionLease{
      .id = id,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = std::string(profile_id),
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
}

ExtensionPermissionLease tab_lease(std::uint64_t id,
                                   std::string_view profile_id) {
  return ExtensionPermissionLease{
      .id = id,
      .grant = ExtensionPermissionGrant{
          .permission = ExtensionPermission::display_notifications,
          .profile_id = std::string(profile_id),
          .scope = ExtensionGrantScope::extension,
          .lifetime = ExtensionGrantLifetime::until_tab_closes,
          .websites = {},
          .private_browsing_allowed = false,
          .active = true,
      },
      .issued_at_millis = 0,
      .expires_at_millis = 0,
      .tab_id = "tab-volatile",
      .browser_session_id = {},
      .revoked = false,
      .consumed = false,
  };
}

}  // namespace

int main() {
  constexpr std::string_view kProfile = "profile-personal";
  constexpr std::uint64_t kIssuedAt = 1000;
  constexpr std::uint64_t kNow = 2000;

  ExtensionPermissionLedger ledger;
  assert(ledger.add(always_lease(1, kProfile)));
  assert(ledger.add(one_hour_lease(2, kProfile, kIssuedAt)));
  assert(ledger.add(once_lease(3, kProfile)));
  assert(ledger.add(tab_lease(4, kProfile)));
  assert(ledger.add(always_lease(5, "profile-work")));

  const auto snapshot = goreecloud::browser::
      encode_extension_permission_ledger_snapshot(ledger, kProfile, kNow);
  assert(snapshot.accepted());

  const auto restored = goreecloud::browser::
      restore_extension_permission_ledger_snapshot(snapshot.bytes, kProfile,
                                                   kNow);
  assert(restored.accepted());
  assert(restored.expired_leases_dropped == 0);
  assert(restored.ledger.leases().size() == 2);
  assert(has_lease_id(restored.ledger, 1));
  assert(has_lease_id(restored.ledger, 2));
  assert(!has_lease_id(restored.ledger, 3));
  assert(!has_lease_id(restored.ledger, 4));
  assert(!has_lease_id(restored.ledger, 5));
  const auto restored_always = std::find_if(
      restored.ledger.leases().begin(), restored.ledger.leases().end(),
      [](const ExtensionPermissionLease& lease) { return lease.id == 1; });
  assert(restored_always != restored.ledger.leases().end());
  assert(restored_always->grant.private_browsing_allowed);

  const auto wrong_profile = goreecloud::browser::
      restore_extension_permission_ledger_snapshot(snapshot.bytes,
                                                   "profile-work", kNow);
  assert(!wrong_profile.accepted());
  assert(has_issue(wrong_profile,
                   ExtensionPermissionLedgerStorageIssue::profile_mismatch));

  auto corrupted = snapshot.bytes;
  corrupted[8] ^= 0x01U;
  const auto corrupted_result = goreecloud::browser::
      restore_extension_permission_ledger_snapshot(corrupted, kProfile, kNow);
  assert(!corrupted_result.accepted());
  assert(has_issue(corrupted_result,
                   ExtensionPermissionLedgerStorageIssue::checksum_mismatch));

  const auto expired_at = kIssuedAt + goreecloud::browser::kExtensionOneHourMillis;
  const auto expired = goreecloud::browser::
      restore_extension_permission_ledger_snapshot(snapshot.bytes, kProfile,
                                                   expired_at);
  assert(expired.accepted());
  assert(expired.expired_leases_dropped == 1);
  assert(expired.ledger.leases().size() == 1);
  assert(has_lease_id(expired.ledger, 1));

  const auto clock_regression = goreecloud::browser::
      restore_extension_permission_ledger_snapshot(snapshot.bytes, kProfile,
                                                   kIssuedAt - 1);
  assert(!clock_regression.accepted());
  assert(has_issue(clock_regression,
                   ExtensionPermissionLedgerStorageIssue::clock_regression));

  const auto path = std::filesystem::temp_directory_path() /
                    "goreecloud-extension-permission-ledger-v1.snapshot";
  const auto temp_path =
      goreecloud::browser::extension_permission_ledger_temp_path(path);
  const auto backup_path =
      goreecloud::browser::extension_permission_ledger_backup_path(path);
  std::error_code error;
  std::filesystem::remove(path, error);
  error.clear();
  std::filesystem::remove(temp_path, error);
  error.clear();
  std::filesystem::remove(backup_path, error);

  const auto first_save = goreecloud::browser::save_extension_permission_ledger(
      path, ledger, kProfile, kNow);
  assert(first_save.empty());
  const auto first_load = goreecloud::browser::load_extension_permission_ledger(
      path, kProfile, kNow);
  assert(first_load.accepted());
  assert(!first_load.recovered_from_backup);
  assert(first_load.ledger.leases().size() == 2);

  ExtensionPermissionLedger replacement;
  assert(replacement.add(always_lease(10, kProfile)));
  const auto second_save = goreecloud::browser::save_extension_permission_ledger(
      path, replacement, kProfile, kNow);
  assert(second_save.empty());
  assert(std::filesystem::exists(backup_path));

  {
    std::fstream output(path,
                        std::ios::binary | std::ios::in | std::ios::out);
    assert(output);
    const char bad_magic = 'X';
    output.seekp(0);
    output.write(&bad_magic, 1);
    output.flush();
    assert(output);
  }

  const auto recovered = goreecloud::browser::load_extension_permission_ledger(
      path, kProfile, kNow);
  assert(recovered.accepted());
  assert(recovered.recovered_from_backup);
  assert(recovered.ledger.leases().size() == 2);
  assert(has_lease_id(recovered.ledger, 1));
  assert(has_lease_id(recovered.ledger, 2));

  error.clear();
  std::filesystem::remove(path, error);
  error.clear();
  std::filesystem::remove(temp_path, error);
  error.clear();
  std::filesystem::remove(backup_path, error);

  return 0;
}
