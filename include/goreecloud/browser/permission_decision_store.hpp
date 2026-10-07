#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

#include "goreecloud/browser/permission_broker.hpp"

namespace goreecloud::browser {

inline constexpr std::uint8_t kPermissionDecisionSnapshotVersion = 1;
inline constexpr std::size_t kPermissionDecisionMaxSnapshotBytes =
    2ULL * 1024ULL * 1024ULL;
inline constexpr std::uint32_t kPermissionDecisionMaxPersistedRecords = 4096;
inline constexpr std::size_t kPermissionDecisionMaxProfileBytes = 128;
inline constexpr std::array<std::uint8_t, 4> kPermissionDecisionSnapshotMagic = {
    static_cast<std::uint8_t>('G'), static_cast<std::uint8_t>('C'),
    static_cast<std::uint8_t>('P'), static_cast<std::uint8_t>('D')};

enum class PermissionDecisionSource {
  user,
};

struct PersistentPermissionDecision {
  std::string profile_id;
  std::string origin;
  PermissionResource resource{PermissionResource::camera};
  PermissionDecision decision{PermissionDecision::deny_persistent};
  PermissionDecisionSource source{PermissionDecisionSource::user};
  std::int64_t created_at_millis{0};
  std::int64_t expires_at_millis{0};
  std::int64_t revoked_at_millis{0};
  std::uint32_t contract_version{1};
};

inline bool persistent_permission_decision_value(PermissionDecision decision) {
  return decision == PermissionDecision::allow_persistent ||
         decision == PermissionDecision::deny_persistent;
}

inline bool valid_persistent_permission_decision(
    const PersistentPermissionDecision& record) {
  if (record.contract_version != 1 ||
      !permission_text_safe(record.profile_id,
                            kPermissionDecisionMaxProfileBytes) ||
      !canonical_web_origin(record.origin) ||
      permission_resource_name(record.resource).empty() ||
      !persistent_permission_decision_value(record.decision) ||
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

inline bool same_permission_decision_key(
    const PersistentPermissionDecision& left,
    const PersistentPermissionDecision& right) {
  return left.profile_id == right.profile_id && left.origin == right.origin &&
         left.resource == right.resource;
}

inline bool identical_persistent_permission_decision(
    const PersistentPermissionDecision& left,
    const PersistentPermissionDecision& right) {
  return same_permission_decision_key(left, right) &&
         left.decision == right.decision && left.source == right.source &&
         left.created_at_millis == right.created_at_millis &&
         left.expires_at_millis == right.expires_at_millis &&
         left.revoked_at_millis == right.revoked_at_millis &&
         left.contract_version == right.contract_version;
}

class PermissionDecisionStore {
 public:
  [[nodiscard]] bool upsert(PersistentPermissionDecision record,
                            PrivacyContext context) {
    if (context != PrivacyContext::normal ||
        !valid_persistent_permission_decision(record)) {
      return false;
    }
    return upsert_validated(std::move(record));
  }

  [[nodiscard]] std::optional<PersistentPermissionDecision> lookup(
      std::string_view profile_id,
      std::string_view origin,
      PermissionResource resource,
      std::int64_t now_millis) const {
    if (now_millis < 0 ||
        !permission_text_safe(profile_id, kPermissionDecisionMaxProfileBytes) ||
        !canonical_web_origin(origin)) {
      return std::nullopt;
    }
    const auto* record = find(profile_id, origin, resource);
    if (record == nullptr || now_millis < record->created_at_millis ||
        (record->revoked_at_millis != 0 &&
         now_millis >= record->revoked_at_millis) ||
        (record->expires_at_millis != 0 &&
         now_millis >= record->expires_at_millis)) {
      return std::nullopt;
    }
    return *record;
  }

  [[nodiscard]] bool revoke(std::string_view profile_id,
                            std::string_view origin,
                            PermissionResource resource,
                            std::int64_t now_millis) {
    auto* record = find_mutable(profile_id, origin, resource);
    if (record == nullptr || now_millis < record->created_at_millis) {
      return false;
    }
    if (record->revoked_at_millis != 0) {
      return now_millis >= record->revoked_at_millis;
    }
    record->revoked_at_millis = now_millis;
    return true;
  }

  [[nodiscard]] std::size_t reset_origin(std::string_view profile_id,
                                         std::string_view origin,
                                         std::int64_t now_millis) {
    if (now_millis < 0 ||
        !permission_text_safe(profile_id, kPermissionDecisionMaxProfileBytes) ||
        !canonical_web_origin(origin)) {
      return 0;
    }
    std::size_t changed = 0;
    for (auto& record : records_) {
      if (record.profile_id != profile_id || record.origin != origin ||
          now_millis < record.created_at_millis ||
          record.revoked_at_millis != 0) {
        continue;
      }
      record.revoked_at_millis = now_millis;
      ++changed;
    }
    return changed;
  }

  [[nodiscard]] const std::vector<PersistentPermissionDecision>& records()
      const noexcept {
    return records_;
  }

 private:
  std::vector<PersistentPermissionDecision> records_;

  [[nodiscard]] const PersistentPermissionDecision* find(
      std::string_view profile_id,
      std::string_view origin,
      PermissionResource resource) const {
    const auto iterator = std::find_if(
        records_.begin(), records_.end(),
        [&](const PersistentPermissionDecision& record) {
          return record.profile_id == profile_id && record.origin == origin &&
                 record.resource == resource;
        });
    return iterator == records_.end() ? nullptr : &*iterator;
  }

  [[nodiscard]] PersistentPermissionDecision* find_mutable(
      std::string_view profile_id,
      std::string_view origin,
      PermissionResource resource) {
    const auto iterator = std::find_if(
        records_.begin(), records_.end(),
        [&](const PersistentPermissionDecision& record) {
          return record.profile_id == profile_id && record.origin == origin &&
                 record.resource == resource;
        });
    return iterator == records_.end() ? nullptr : &*iterator;
  }

  [[nodiscard]] bool upsert_validated(PersistentPermissionDecision record) {
    auto* existing =
        find_mutable(record.profile_id, record.origin, record.resource);
    if (existing == nullptr) {
      if (records_.size() >= kPermissionDecisionMaxPersistedRecords) {
        return false;
      }
      records_.push_back(std::move(record));
      return true;
    }

    if (record.created_at_millis < existing->created_at_millis ||
        (existing->revoked_at_millis != 0 &&
         record.created_at_millis <= existing->revoked_at_millis)) {
      return false;
    }
    if (record.created_at_millis == existing->created_at_millis) {
      return identical_persistent_permission_decision(*existing, record);
    }
    *existing = std::move(record);
    return true;
  }

  friend struct PermissionDecisionStoreRestoreAccess;
};

struct PermissionDecisionStoreRestoreAccess {
  static bool add(PermissionDecisionStore& store,
                  PersistentPermissionDecision record) {
    if (!valid_persistent_permission_decision(record)) {
      return false;
    }
    return store.upsert_validated(std::move(record));
  }
};

enum class PermissionDecisionStorageIssue {
  invalid_profile,
  invalid_time,
  snapshot_too_large,
  invalid_magic,
  unsupported_version,
  invalid_reserved_header,
  truncated_snapshot,
  checksum_mismatch,
  profile_mismatch,
  invalid_record_count,
  duplicate_record,
  invalid_enum_value,
  invalid_record,
  clock_regression,
  trailing_snapshot_bytes,
  io_error,
};

struct PermissionDecisionSnapshotResult {
  std::vector<std::uint8_t> bytes;
  std::vector<PermissionDecisionStorageIssue> issues;

  [[nodiscard]] bool accepted() const noexcept {
    return !bytes.empty() && issues.empty();
  }
};

struct PermissionDecisionRestoreResult {
  PermissionDecisionStore store;
  std::vector<PermissionDecisionStorageIssue> issues;
  std::size_t expired_records_dropped{0};
  bool recovered_from_backup{false};

  [[nodiscard]] bool accepted() const noexcept { return issues.empty(); }
};

inline void add_permission_decision_storage_issue_once(
    std::vector<PermissionDecisionStorageIssue>& issues,
    PermissionDecisionStorageIssue issue) {
  if (std::find(issues.begin(), issues.end(), issue) == issues.end()) {
    issues.push_back(issue);
  }
}

inline std::uint32_t permission_decision_crc32(
    std::span<const std::uint8_t> bytes) {
  std::uint32_t crc = 0xffffffffU;
  for (const auto byte : bytes) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit) {
      const auto mask = static_cast<std::uint32_t>(
          0U - static_cast<std::uint32_t>(crc & 1U));
      crc = (crc >> 1U) ^ (0xedb88320U & mask);
    }
  }
  return ~crc;
}

class PermissionDecisionByteWriter {
 public:
  void write_u8(std::uint8_t value) { bytes_.push_back(value); }

  void write_u16(std::uint16_t value) {
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  }

  void write_u32(std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
      bytes_.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
  }

  void write_u64(std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
      bytes_.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
  }

  bool write_string_u16(std::string_view value, std::size_t maximum) {
    if (value.size() > maximum ||
        value.size() > std::numeric_limits<std::uint16_t>::max()) {
      return false;
    }
    write_u16(static_cast<std::uint16_t>(value.size()));
    bytes_.insert(bytes_.end(), value.begin(), value.end());
    return true;
  }

  void write_bytes(std::span<const std::uint8_t> value) {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept {
    return bytes_;
  }
  [[nodiscard]] std::vector<std::uint8_t> take() { return std::move(bytes_); }

 private:
  std::vector<std::uint8_t> bytes_;
};

class PermissionDecisionByteReader {
 public:
  explicit PermissionDecisionByteReader(std::span<const std::uint8_t> bytes)
      : bytes_(bytes) {}

  [[nodiscard]] std::size_t remaining() const noexcept {
    return bytes_.size() - offset_;
  }
  [[nodiscard]] bool at_end() const noexcept { return offset_ == bytes_.size(); }

  bool read_u8(std::uint8_t& value) {
    if (remaining() < 1) return false;
    value = bytes_[offset_++];
    return true;
  }
  bool read_u16(std::uint16_t& value) {
    if (remaining() < 2) return false;
    value = static_cast<std::uint16_t>(bytes_[offset_]) |
            (static_cast<std::uint16_t>(bytes_[offset_ + 1]) << 8U);
    offset_ += 2;
    return true;
  }
  bool read_u32(std::uint32_t& value) {
    if (remaining() < 4) return false;
    value = static_cast<std::uint32_t>(bytes_[offset_]) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 1]) << 8U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 2]) << 16U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 3]) << 24U);
    offset_ += 4;
    return true;
  }
  bool read_u64(std::uint64_t& value) {
    if (remaining() < 8) return false;
    value = 0;
    for (std::size_t index = 0; index < 8; ++index) {
      value |= static_cast<std::uint64_t>(bytes_[offset_ + index])
               << (index * 8U);
    }
    offset_ += 8;
    return true;
  }
  bool read_string_u16(std::string& value, std::size_t maximum) {
    std::uint16_t length = 0;
    if (!read_u16(length) || length > maximum || length > remaining()) {
      return false;
    }
    value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_),
                 length);
    offset_ += length;
    return true;
  }
  bool read_bytes(std::size_t length,
                  std::span<const std::uint8_t>& value) {
    if (length > remaining()) return false;
    value = bytes_.subspan(offset_, length);
    offset_ += length;
    return true;
  }

 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t offset_{0};
};

inline std::uint8_t permission_resource_storage_code(PermissionResource value) {
  return static_cast<std::uint8_t>(value) + 1U;
}
inline std::optional<PermissionResource> parse_permission_resource_storage_code(
    std::uint8_t code) {
  if (code < 1U || code > 5U) return std::nullopt;
  return static_cast<PermissionResource>(code - 1U);
}
inline std::uint8_t permission_decision_storage_code(PermissionDecision value) {
  return value == PermissionDecision::allow_persistent ? 1U : 2U;
}
inline std::optional<PermissionDecision> parse_permission_decision_storage_code(
    std::uint8_t code) {
  if (code == 1U) return PermissionDecision::allow_persistent;
  if (code == 2U) return PermissionDecision::deny_persistent;
  return std::nullopt;
}

inline PermissionDecisionSnapshotResult encode_permission_decision_snapshot(
    const PermissionDecisionStore& store,
    std::string_view profile_id,
    std::int64_t now_millis) {
  PermissionDecisionSnapshotResult result;
  if (!permission_text_safe(profile_id, kPermissionDecisionMaxProfileBytes)) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_profile);
    return result;
  }
  if (now_millis < 0) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_time);
    return result;
  }

  std::vector<const PersistentPermissionDecision*> records;
  for (const auto& record : store.records()) {
    if (record.profile_id == profile_id) records.push_back(&record);
  }
  if (records.size() > kPermissionDecisionMaxPersistedRecords) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_record_count);
    return result;
  }

  PermissionDecisionByteWriter writer;
  writer.write_bytes(kPermissionDecisionSnapshotMagic);
  writer.write_u8(kPermissionDecisionSnapshotVersion);
  writer.write_u8(0U);
  writer.write_u8(0U);
  writer.write_u8(0U);
  writer.write_u64(static_cast<std::uint64_t>(now_millis));
  if (!writer.write_string_u16(profile_id, kPermissionDecisionMaxProfileBytes)) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_profile);
    return result;
  }
  writer.write_u32(static_cast<std::uint32_t>(records.size()));

  for (const auto* record : records) {
    if (!valid_persistent_permission_decision(*record) ||
        !writer.write_string_u16(record->origin, 2048)) {
      result.issues.push_back(PermissionDecisionStorageIssue::invalid_record);
      return result;
    }
    writer.write_u8(permission_resource_storage_code(record->resource));
    writer.write_u8(permission_decision_storage_code(record->decision));
    writer.write_u8(1U);  // user decision source
    writer.write_u8(0U);  // reserved
    writer.write_u64(static_cast<std::uint64_t>(record->created_at_millis));
    writer.write_u64(static_cast<std::uint64_t>(record->expires_at_millis));
    writer.write_u64(static_cast<std::uint64_t>(record->revoked_at_millis));
  }

  if (writer.bytes().size() + sizeof(std::uint32_t) >
      kPermissionDecisionMaxSnapshotBytes) {
    result.issues.push_back(PermissionDecisionStorageIssue::snapshot_too_large);
    return result;
  }
  const auto checksum = permission_decision_crc32(writer.bytes());
  writer.write_u32(checksum);
  result.bytes = writer.take();
  return result;
}

inline PermissionDecisionRestoreResult restore_permission_decision_snapshot(
    std::span<const std::uint8_t> bytes,
    std::string_view expected_profile_id,
    std::int64_t now_millis) {
  PermissionDecisionRestoreResult result;
  if (!permission_text_safe(expected_profile_id,
                            kPermissionDecisionMaxProfileBytes)) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_profile);
    return result;
  }
  if (now_millis < 0) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_time);
    return result;
  }
  if (bytes.size() > kPermissionDecisionMaxSnapshotBytes) {
    result.issues.push_back(PermissionDecisionStorageIssue::snapshot_too_large);
    return result;
  }
  constexpr std::size_t kMinimumSnapshotBytes =
      4U + 1U + 3U + 8U + 2U + 4U + 4U;
  if (bytes.size() < kMinimumSnapshotBytes) {
    result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
    return result;
  }

  const auto payload_size = bytes.size() - sizeof(std::uint32_t);
  const auto payload = bytes.first(payload_size);
  PermissionDecisionByteReader checksum_reader(bytes.subspan(payload_size));
  std::uint32_t stored_checksum = 0;
  if (!checksum_reader.read_u32(stored_checksum) ||
      stored_checksum != permission_decision_crc32(payload)) {
    result.issues.push_back(PermissionDecisionStorageIssue::checksum_mismatch);
    return result;
  }

  PermissionDecisionByteReader reader(payload);
  std::span<const std::uint8_t> magic;
  if (!reader.read_bytes(kPermissionDecisionSnapshotMagic.size(), magic)) {
    result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
    return result;
  }
  if (!std::equal(magic.begin(), magic.end(),
                  kPermissionDecisionSnapshotMagic.begin())) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_magic);
    return result;
  }

  std::uint8_t version = 0;
  if (!reader.read_u8(version)) {
    result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
    return result;
  }
  if (version != kPermissionDecisionSnapshotVersion) {
    result.issues.push_back(PermissionDecisionStorageIssue::unsupported_version);
    return result;
  }
  for (int index = 0; index < 3; ++index) {
    std::uint8_t reserved = 0;
    if (!reader.read_u8(reserved)) {
      result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
      return result;
    }
    if (reserved != 0U) {
      result.issues.push_back(
          PermissionDecisionStorageIssue::invalid_reserved_header);
      return result;
    }
  }

  std::uint64_t snapshot_time = 0;
  if (!reader.read_u64(snapshot_time)) {
    result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
    return result;
  }
  if (static_cast<std::uint64_t>(now_millis) < snapshot_time) {
    result.issues.push_back(PermissionDecisionStorageIssue::clock_regression);
    return result;
  }

  std::string profile_id;
  if (!reader.read_string_u16(profile_id, kPermissionDecisionMaxProfileBytes) ||
      !permission_text_safe(profile_id, kPermissionDecisionMaxProfileBytes)) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_profile);
    return result;
  }
  if (profile_id != expected_profile_id) {
    result.issues.push_back(PermissionDecisionStorageIssue::profile_mismatch);
    return result;
  }

  std::uint32_t record_count = 0;
  if (!reader.read_u32(record_count)) {
    result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
    return result;
  }
  if (record_count > kPermissionDecisionMaxPersistedRecords) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_record_count);
    return result;
  }

  std::unordered_set<std::string> seen_keys;
  for (std::uint32_t index = 0; index < record_count; ++index) {
    std::string origin;
    std::uint8_t resource_code = 0;
    std::uint8_t decision_code = 0;
    std::uint8_t source_code = 0;
    std::uint8_t reserved = 0;
    std::uint64_t created_at = 0;
    std::uint64_t expires_at = 0;
    std::uint64_t revoked_at = 0;
    if (!reader.read_string_u16(origin, 2048) ||
        !reader.read_u8(resource_code) || !reader.read_u8(decision_code) ||
        !reader.read_u8(source_code) || !reader.read_u8(reserved) ||
        !reader.read_u64(created_at) || !reader.read_u64(expires_at) ||
        !reader.read_u64(revoked_at)) {
      result.issues.push_back(PermissionDecisionStorageIssue::truncated_snapshot);
      return result;
    }
    if (source_code != 1U || reserved != 0U) {
      result.issues.push_back(PermissionDecisionStorageIssue::invalid_enum_value);
      return result;
    }
    const auto resource = parse_permission_resource_storage_code(resource_code);
    const auto decision = parse_permission_decision_storage_code(decision_code);
    if (!resource.has_value() || !decision.has_value() ||
        created_at > static_cast<std::uint64_t>(
                         std::numeric_limits<std::int64_t>::max()) ||
        expires_at > static_cast<std::uint64_t>(
                         std::numeric_limits<std::int64_t>::max()) ||
        revoked_at > static_cast<std::uint64_t>(
                         std::numeric_limits<std::int64_t>::max())) {
      result.issues.push_back(PermissionDecisionStorageIssue::invalid_enum_value);
      return result;
    }

    const std::string key = origin + "\n" + std::to_string(resource_code);
    if (!seen_keys.insert(key).second) {
      result.issues.push_back(PermissionDecisionStorageIssue::duplicate_record);
      return result;
    }

    PersistentPermissionDecision record{
        .profile_id = profile_id,
        .origin = std::move(origin),
        .resource = *resource,
        .decision = *decision,
        .source = PermissionDecisionSource::user,
        .created_at_millis = static_cast<std::int64_t>(created_at),
        .expires_at_millis = static_cast<std::int64_t>(expires_at),
        .revoked_at_millis = static_cast<std::int64_t>(revoked_at),
        .contract_version = 1,
    };
    if (!valid_persistent_permission_decision(record)) {
      result.issues.push_back(PermissionDecisionStorageIssue::invalid_record);
      return result;
    }
    if (record.revoked_at_millis == 0 && record.expires_at_millis != 0 &&
        now_millis >= record.expires_at_millis) {
      ++result.expired_records_dropped;
      continue;
    }
    if (!PermissionDecisionStoreRestoreAccess::add(result.store,
                                                    std::move(record))) {
      result.issues.push_back(PermissionDecisionStorageIssue::invalid_record);
      return result;
    }
  }

  if (!reader.at_end()) {
    result.issues.push_back(
        PermissionDecisionStorageIssue::trailing_snapshot_bytes);
  }
  return result;
}

inline std::filesystem::path permission_decision_temp_path(
    const std::filesystem::path& path) {
  auto result = path;
  result += ".tmp";
  return result;
}
inline std::filesystem::path permission_decision_backup_path(
    const std::filesystem::path& path) {
  auto result = path;
  result += ".bak";
  return result;
}

inline bool read_permission_decision_snapshot_file(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& bytes) {
  std::error_code error;
  const auto file_size = std::filesystem::file_size(path, error);
  if (error || file_size > kPermissionDecisionMaxSnapshotBytes ||
      file_size > static_cast<std::uintmax_t>(
                      std::numeric_limits<std::size_t>::max())) {
    return false;
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  bytes.resize(static_cast<std::size_t>(file_size));
  if (!bytes.empty()) {
    input.read(reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!input || static_cast<std::size_t>(input.gcount()) != bytes.size()) {
      return false;
    }
  }
  return true;
}

inline bool write_permission_decision_snapshot_file(
    const std::filesystem::path& path,
    std::span<const std::uint8_t> bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) return false;
  if (!bytes.empty()) {
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
  }
  output.flush();
  return static_cast<bool>(output);
}

inline std::vector<PermissionDecisionStorageIssue> save_permission_decisions(
    const std::filesystem::path& path,
    const PermissionDecisionStore& store,
    std::string_view profile_id,
    std::int64_t now_millis) {
  auto snapshot = encode_permission_decision_snapshot(store, profile_id,
                                                      now_millis);
  if (!snapshot.accepted()) return snapshot.issues;

  const auto temp_path = permission_decision_temp_path(path);
  const auto backup_path = permission_decision_backup_path(path);
  std::error_code error;
  std::filesystem::remove(temp_path, error);
  error.clear();
  if (!write_permission_decision_snapshot_file(temp_path, snapshot.bytes)) {
    std::filesystem::remove(temp_path, error);
    return {PermissionDecisionStorageIssue::io_error};
  }

  const bool primary_exists = std::filesystem::exists(path, error) && !error;
  bool primary_valid = false;
  if (primary_exists) {
    std::vector<std::uint8_t> existing;
    if (read_permission_decision_snapshot_file(path, existing)) {
      auto primary = restore_permission_decision_snapshot(
          existing, profile_id, now_millis);
      primary_valid = primary.accepted();
      if (!primary_valid &&
          (std::find(primary.issues.begin(), primary.issues.end(),
                     PermissionDecisionStorageIssue::clock_regression) !=
               primary.issues.end() ||
           std::find(primary.issues.begin(), primary.issues.end(),
                     PermissionDecisionStorageIssue::profile_mismatch) !=
               primary.issues.end() ||
           std::find(primary.issues.begin(), primary.issues.end(),
                     PermissionDecisionStorageIssue::unsupported_version) !=
               primary.issues.end())) {
        std::filesystem::remove(temp_path, error);
        return primary.issues;
      }
    }
  }
  error.clear();

  if (primary_valid) {
    std::filesystem::remove(backup_path, error);
    error.clear();
    std::filesystem::rename(path, backup_path, error);
    if (error) {
      std::filesystem::remove(temp_path, error);
      return {PermissionDecisionStorageIssue::io_error};
    }
  } else if (primary_exists) {
    std::filesystem::remove(path, error);
    if (error) {
      std::filesystem::remove(temp_path, error);
      return {PermissionDecisionStorageIssue::io_error};
    }
  }

  error.clear();
  std::filesystem::rename(temp_path, path, error);
  if (error) {
    if (primary_valid) {
      std::error_code restore_error;
      std::filesystem::rename(backup_path, path, restore_error);
    }
    std::filesystem::remove(temp_path, error);
    return {PermissionDecisionStorageIssue::io_error};
  }
  return {};
}

inline PermissionDecisionRestoreResult load_permission_decisions(
    const std::filesystem::path& path,
    std::string_view expected_profile_id,
    std::int64_t now_millis) {
  PermissionDecisionRestoreResult result;
  if (!permission_text_safe(expected_profile_id,
                            kPermissionDecisionMaxProfileBytes)) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_profile);
    return result;
  }
  if (now_millis < 0) {
    result.issues.push_back(PermissionDecisionStorageIssue::invalid_time);
    return result;
  }

  std::error_code error;
  const bool primary_exists = std::filesystem::exists(path, error);
  if (error) {
    result.issues.push_back(PermissionDecisionStorageIssue::io_error);
    return result;
  }
  const auto backup_path = permission_decision_backup_path(path);
  const bool backup_exists = std::filesystem::exists(backup_path, error);
  if (error) {
    result.issues.push_back(PermissionDecisionStorageIssue::io_error);
    return result;
  }
  if (!primary_exists && !backup_exists) return result;

  std::optional<PermissionDecisionRestoreResult> primary_failure;
  if (primary_exists) {
    std::vector<std::uint8_t> bytes;
    if (!read_permission_decision_snapshot_file(path, bytes)) {
      PermissionDecisionRestoreResult failed;
      failed.issues.push_back(PermissionDecisionStorageIssue::io_error);
      primary_failure = std::move(failed);
    } else {
      auto primary = restore_permission_decision_snapshot(
          bytes, expected_profile_id, now_millis);
      if (primary.accepted()) return primary;
      primary_failure = std::move(primary);
    }
  }

  if (backup_exists) {
    std::vector<std::uint8_t> bytes;
    if (read_permission_decision_snapshot_file(backup_path, bytes)) {
      auto backup = restore_permission_decision_snapshot(
          bytes, expected_profile_id, now_millis);
      if (backup.accepted()) {
        backup.recovered_from_backup = true;
        return backup;
      }
      if (!primary_failure.has_value()) return backup;
    } else if (!primary_failure.has_value()) {
      result.issues.push_back(PermissionDecisionStorageIssue::io_error);
      return result;
    }
  }

  if (primary_failure.has_value()) return std::move(*primary_failure);
  result.issues.push_back(PermissionDecisionStorageIssue::io_error);
  return result;
}

}  // namespace goreecloud::browser
