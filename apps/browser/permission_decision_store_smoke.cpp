#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

#include "goreecloud/browser/permission_decision_store.hpp"

using goreecloud::browser::PermissionDecision;
using goreecloud::browser::PermissionDecisionSource;
using goreecloud::browser::PermissionDecisionStorageIssue;
using goreecloud::browser::PermissionDecisionStore;
using goreecloud::browser::PermissionResource;
using goreecloud::browser::PersistentPermissionDecision;
using goreecloud::browser::PrivacyContext;

namespace {

void require(bool condition) {
  if (!condition) std::abort();
}

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

bool has_issue(
    const std::vector<PermissionDecisionStorageIssue>& issues,
    PermissionDecisionStorageIssue issue) {
  return std::find(issues.begin(), issues.end(), issue) != issues.end();
}

void append_u32_little_endian(std::vector<std::uint8_t>& bytes,
                              std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
}

std::vector<std::uint8_t> duplicate_first_snapshot_record(
    const std::vector<std::uint8_t>& snapshot,
    std::string_view profile_id) {
  require(snapshot.size() > 4);
  const std::size_t count_offset =
      4U + 1U + 3U + 8U + 2U + profile_id.size();
  const std::size_t record_offset = count_offset + 4U;
  const std::size_t payload_size = snapshot.size() - 4U;
  require(record_offset < payload_size);

  std::vector<std::uint8_t> duplicated(snapshot.begin(),
                                       snapshot.begin() + payload_size);
  duplicated[count_offset] = 2U;
  duplicated[count_offset + 1U] = 0U;
  duplicated[count_offset + 2U] = 0U;
  duplicated[count_offset + 3U] = 0U;
  duplicated.insert(duplicated.end(),
                    snapshot.begin() + record_offset,
                    snapshot.begin() + payload_size);
  const auto checksum = goreecloud::browser::permission_decision_crc32(
      duplicated);
  append_u32_little_endian(duplicated, checksum);
  return duplicated;
}

}  // namespace

int main() {
  PermissionDecisionStore store;
  require(store.upsert(decision(), PrivacyContext::normal));
  const auto active = store.lookup("profile-personal", "https://example.test",
                                   PermissionResource::camera, 1500);
  require(active.has_value());
  require(active->decision == PermissionDecision::allow_persistent);

  auto private_record = decision(PermissionResource::microphone);
  require(!store.upsert(private_record, PrivacyContext::private_browsing));
  require(!store.upsert(private_record, PrivacyContext::isolated_private));

  auto transient = decision(PermissionResource::microphone,
                            PermissionDecision::allow_session);
  require(!store.upsert(transient, PrivacyContext::normal));

  auto expiring = decision(PermissionResource::geolocation,
                           PermissionDecision::deny_persistent, 1200, 2000);
  require(store.upsert(expiring, PrivacyContext::normal));
  require(store.lookup("profile-personal", "https://example.test",
                      PermissionResource::geolocation, 1999)
             .has_value());
  require(!store.lookup("profile-personal", "https://example.test",
                       PermissionResource::geolocation, 2000)
              .has_value());

  require(store.revoke("profile-personal", "https://example.test",
                      PermissionResource::camera, 1600));
  require(!store.lookup("profile-personal", "https://example.test",
                       PermissionResource::camera, 1600)
              .has_value());
  require(store.revoke("profile-personal", "https://example.test",
                      PermissionResource::camera, 1700));

  auto stale = decision(PermissionResource::camera,
                        PermissionDecision::deny_persistent, 1500);
  require(!store.upsert(stale, PrivacyContext::normal));
  auto fresh = decision(PermissionResource::camera,
                        PermissionDecision::deny_persistent, 1701);
  require(store.upsert(fresh, PrivacyContext::normal));
  require(store.lookup("profile-personal", "https://example.test",
                      PermissionResource::camera, 1800)
             .has_value());

  require(store.reset_origin("profile-personal", "https://example.test", 1900) ==
         2);
  require(!store.lookup("profile-personal", "https://example.test",
                       PermissionResource::camera, 1900)
              .has_value());

  PermissionDecisionStore durable;
  auto persistent = decision(PermissionResource::camera,
                             PermissionDecision::allow_persistent, 1000);
  require(durable.upsert(persistent, PrivacyContext::normal));
  auto expires = decision(PermissionResource::microphone,
                          PermissionDecision::deny_persistent, 1000, 1800);
  require(durable.upsert(expires, PrivacyContext::normal));
  auto revoked = decision(PermissionResource::geolocation,
                          PermissionDecision::allow_persistent, 1000);
  require(durable.upsert(revoked, PrivacyContext::normal));
  require(durable.revoke("profile-personal", "https://example.test",
                        PermissionResource::geolocation, 1300));

  const auto snapshot = goreecloud::browser::encode_permission_decision_snapshot(
      durable, "profile-personal", 1500);
  require(snapshot.accepted());
  PermissionDecisionStore one_record;
  require(one_record.upsert(decision(), PrivacyContext::normal));
  const auto one_record_snapshot =
      goreecloud::browser::encode_permission_decision_snapshot(
          one_record, "profile-personal", 1500);
  require(one_record_snapshot.accepted());
  const auto duplicated_snapshot = duplicate_first_snapshot_record(
      one_record_snapshot.bytes, "profile-personal");
  const auto duplicate_result =
      goreecloud::browser::restore_permission_decision_snapshot(
          duplicated_snapshot, "profile-personal", 1500);
  require(!duplicate_result.accepted());
  require(has_issue(duplicate_result,
                   PermissionDecisionStorageIssue::duplicate_record));

  const auto restored = goreecloud::browser::restore_permission_decision_snapshot(
      snapshot.bytes, "profile-personal", 1500);
  require(restored.accepted());
  require(restored.store.records().size() == 3);

  const auto after_expiry =
      goreecloud::browser::restore_permission_decision_snapshot(
          snapshot.bytes, "profile-personal", 1800);
  require(after_expiry.accepted());
  require(after_expiry.expired_records_dropped == 1);
  require(after_expiry.store.records().size() == 2);

  const auto wrong_profile =
      goreecloud::browser::restore_permission_decision_snapshot(
          snapshot.bytes, "profile-work", 1500);
  require(!wrong_profile.accepted());
  require(has_issue(wrong_profile,
                   PermissionDecisionStorageIssue::profile_mismatch));

  const auto clock_regression =
      goreecloud::browser::restore_permission_decision_snapshot(
          snapshot.bytes, "profile-personal", 1499);
  require(!clock_regression.accepted());
  require(has_issue(clock_regression,
                   PermissionDecisionStorageIssue::clock_regression));

  auto corrupted = snapshot.bytes;
  corrupted[8] ^= 0x01U;
  const auto corrupted_result =
      goreecloud::browser::restore_permission_decision_snapshot(
          corrupted, "profile-personal", 1500);
  require(!corrupted_result.accepted());
  require(has_issue(corrupted_result,
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
  require(first_run.accepted());
  require(first_run.store.records().empty());

  const auto first_save = goreecloud::browser::save_permission_decisions(
      path, durable, "profile-personal", 1500);
  require(first_save.empty());
  const auto first_load = goreecloud::browser::load_permission_decisions(
      path, "profile-personal", 1500);
  require(first_load.accepted());
  require(first_load.store.records().size() == 3);

  std::vector<std::uint8_t> authoritative_bytes;
  require(goreecloud::browser::read_permission_decision_snapshot_file(
      path, authoritative_bytes));

  const auto regressed_save = goreecloud::browser::save_permission_decisions(
      path, durable, "profile-personal", 1499);
  require(has_issue(regressed_save,
                    PermissionDecisionStorageIssue::clock_regression));
  std::vector<std::uint8_t> after_regressed_save;
  require(goreecloud::browser::read_permission_decision_snapshot_file(
      path, after_regressed_save));
  require(authoritative_bytes == after_regressed_save);

  PermissionDecisionStore other_profile;
  auto other_profile_record = decision(PermissionResource::camera);
  other_profile_record.profile_id = "profile-work";
  require(other_profile.upsert(other_profile_record, PrivacyContext::normal));
  const auto profile_mismatch_save =
      goreecloud::browser::save_permission_decisions(
          path, other_profile, "profile-work", 1600);
  require(has_issue(profile_mismatch_save,
                    PermissionDecisionStorageIssue::profile_mismatch));
  std::vector<std::uint8_t> after_profile_mismatch_save;
  require(goreecloud::browser::read_permission_decision_snapshot_file(
      path, after_profile_mismatch_save));
  require(authoritative_bytes == after_profile_mismatch_save);

  PermissionDecisionStore replacement;
  auto replacement_record = decision(PermissionResource::protected_media,
                                     PermissionDecision::deny_persistent, 1600);
  require(replacement.upsert(replacement_record, PrivacyContext::normal));
  const auto second_save = goreecloud::browser::save_permission_decisions(
      path, replacement, "profile-personal", 1700);
  require(second_save.empty());
  require(std::filesystem::exists(backup_path));

  {
    std::fstream output(path,
                        std::ios::binary | std::ios::in | std::ios::out);
    require(output);
    const char bad_magic = 'X';
    output.seekp(0);
    output.write(&bad_magic, 1);
    output.flush();
    require(output);
  }

  const auto recovered = goreecloud::browser::load_permission_decisions(
      path, "profile-personal", 1700);
  require(recovered.accepted());
  require(recovered.recovered_from_backup);
  require(recovered.store.records().size() == 3);

  error.clear();
  std::filesystem::remove(path, error);
  error.clear();
  std::filesystem::remove(temp_path, error);
  error.clear();
  std::filesystem::remove(backup_path, error);

  return 0;
}
