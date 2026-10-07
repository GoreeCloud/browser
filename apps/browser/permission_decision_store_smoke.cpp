#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "goreecloud/browser/permission_decision_store.hpp"

using goreecloud::browser::PermissionDecision;
using goreecloud::browser::PermissionDecisionSource;
using goreecloud::browser::PermissionDecisionStorageIssue;
using goreecloud::browser::PermissionDecisionStore;
using goreecloud::browser::PermissionResource;
using goreecloud::browser::PersistentPermissionDecision;
using goreecloud::browser::PrivacyContext;

namespace {

PersistentPermissionDecision decision(
    PermissionResource resource = PermissionResource::camera,
    PermissionDecision value = PermissionDecision::allow_persistent,
    std::int64_t created_at = 1000,
    std::int64_t expires_at = 0) {
  return PersistentPermissionDecision{
      .profile_id = "profile-personal",
      .origin = "https://example.test",
      .resource = resource,
      .decision = value,
      .source = PermissionDecisionSource::user,
      .created_at_millis = created_at,
      .expires_at_millis = expires_at,
      .revoked_at_millis = 0,
      .contract_version = 1,
  };
}

bool has_issue(const goreecloud::browser::PermissionDecisionRestoreResult& result,
               PermissionDecisionStorageIssue issue) {
  return std::find(result.issues.begin(), result.issues.end(), issue) !=
         result.issues.end();
}

}  // namespace

int main() {
  PermissionDecisionStore store;
  assert(store.upsert(decision(), PrivacyContext::normal));
  const auto active = store.lookup("profile-personal", "https://example.test",
                                   PermissionResource::camera, 1500);
  assert(active.has_value());
  assert(active->decision == PermissionDecision::allow_persistent);

  auto private_record = decision(PermissionResource::microphone);
  assert(!store.upsert(private_record, PrivacyContext::private_browsing));
  assert(!store.upsert(private_record, PrivacyContext::isolated_private));

  auto transient = decision(PermissionResource::microphone,
                            PermissionDecision::allow_session);
  assert(!store.upsert(transient, PrivacyContext::normal));

  auto expiring = decision(PermissionResource::geolocation,
                           PermissionDecision::deny_persistent, 1200, 2000);
  assert(store.upsert(expiring, PrivacyContext::normal));
  assert(store.lookup("profile-personal", "https://example.test",
                      PermissionResource::geolocation, 1999)
             .has_value());
  assert(!store.lookup("profile-personal", "https://example.test",
                       PermissionResource::geolocation, 2000)
              .has_value());

  assert(store.revoke("profile-personal", "https://example.test",
                      PermissionResource::camera, 1600));
  assert(!store.lookup("profile-personal", "https://example.test",
                       PermissionResource::camera, 1600)
              .has_value());
  assert(store.revoke("profile-personal", "https://example.test",
                      PermissionResource::camera, 1700));

  auto stale = decision(PermissionResource::camera,
                        PermissionDecision::deny_persistent, 1500);
  assert(!store.upsert(stale, PrivacyContext::normal));
  auto fresh = decision(PermissionResource::camera,
                        PermissionDecision::deny_persistent, 1701);
  assert(store.upsert(fresh, PrivacyContext::normal));
  assert(store.lookup("profile-personal", "https://example.test",
                      PermissionResource::camera, 1800)
             .has_value());

  assert(store.reset_origin("profile-personal", "https://example.test", 1900) ==
         2);
  assert(!store.lookup("profile-personal", "https://example.test",
                       PermissionResource::camera, 1900)
              .has_value());

  PermissionDecisionStore durable;
  auto persistent = decision(PermissionResource::camera,
                             PermissionDecision::allow_persistent, 1000);
  assert(durable.upsert(persistent, PrivacyContext::normal));
  auto expires = decision(PermissionResource::microphone,
                          PermissionDecision::deny_persistent, 1000, 1800);
  assert(durable.upsert(expires, PrivacyContext::normal));
  auto revoked = decision(PermissionResource::geolocation,
                          PermissionDecision::allow_persistent, 1000);
  assert(durable.upsert(revoked, PrivacyContext::normal));
  assert(durable.revoke("profile-personal", "https://example.test",
                        PermissionResource::geolocation, 1300));

  const auto snapshot = goreecloud::browser::encode_permission_decision_snapshot(
      durable, "profile-personal", 1500);
  assert(snapshot.accepted());
  const auto restored = goreecloud::browser::restore_permission_decision_snapshot(
      snapshot.bytes, "profile-personal", 1500);
  assert(restored.accepted());
  assert(restored.store.records().size() == 3);

  const auto after_expiry =
      goreecloud::browser::restore_permission_decision_snapshot(
          snapshot.bytes, "profile-personal", 1800);
  assert(after_expiry.accepted());
  assert(after_expiry.expired_records_dropped == 1);
  assert(after_expiry.store.records().size() == 2);

  const auto wrong_profile =
      goreecloud::browser::restore_permission_decision_snapshot(
          snapshot.bytes, "profile-work", 1500);
  assert(!wrong_profile.accepted());
  assert(has_issue(wrong_profile,
                   PermissionDecisionStorageIssue::profile_mismatch));

  const auto clock_regression =
      goreecloud::browser::restore_permission_decision_snapshot(
          snapshot.bytes, "profile-personal", 1499);
  assert(!clock_regression.accepted());
  assert(has_issue(clock_regression,
                   PermissionDecisionStorageIssue::clock_regression));

  auto corrupted = snapshot.bytes;
  corrupted[8] ^= 0x01U;
  const auto corrupted_result =
      goreecloud::browser::restore_permission_decision_snapshot(
          corrupted, "profile-personal", 1500);
  assert(!corrupted_result.accepted());
  assert(has_issue(corrupted_result,
                   PermissionDecisionStorageIssue::checksum_mismatch));

  const auto path = std::filesystem::temp_directory_path() /
                    "goreecloud-browser-permission-decisions-v1.snapshot";
  const auto temp_path = goreecloud::browser::permission_decision_temp_path(path);
  const auto backup_path =
      goreecloud::browser::permission_decision_backup_path(path);
  std::error_code error;
  std::filesystem::remove(path, error);
  error.clear();
  std::filesystem::remove(temp_path, error);
  error.clear();
  std::filesystem::remove(backup_path, error);

  const auto first_run = goreecloud::browser::load_permission_decisions(
      path, "profile-personal", 1500);
  assert(first_run.accepted());
  assert(first_run.store.records().empty());

  const auto first_save = goreecloud::browser::save_permission_decisions(
      path, durable, "profile-personal", 1500);
  assert(first_save.empty());
  const auto first_load = goreecloud::browser::load_permission_decisions(
      path, "profile-personal", 1500);
  assert(first_load.accepted());
  assert(first_load.store.records().size() == 3);

  PermissionDecisionStore replacement;
  auto replacement_record = decision(PermissionResource::protected_media,
                                     PermissionDecision::deny_persistent, 1600);
  assert(replacement.upsert(replacement_record, PrivacyContext::normal));
  const auto second_save = goreecloud::browser::save_permission_decisions(
      path, replacement, "profile-personal", 1700);
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

  const auto recovered = goreecloud::browser::load_permission_decisions(
      path, "profile-personal", 1700);
  assert(recovered.accepted());
  assert(recovered.recovered_from_backup);
  assert(recovered.store.records().size() == 3);

  error.clear();
  std::filesystem::remove(path, error);
  error.clear();
  std::filesystem::remove(temp_path, error);
  error.clear();
  std::filesystem::remove(backup_path, error);

  return 0;
}
