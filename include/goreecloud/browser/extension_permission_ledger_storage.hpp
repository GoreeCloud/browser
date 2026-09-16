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

#include "goreecloud/browser/extension_permission_ledger.hpp"

namespace goreecloud::browser {

inline constexpr std::uint8_t kExtensionPermissionLedgerSnapshotVersion = 1;
inline constexpr std::size_t kExtensionPermissionLedgerMaxSnapshotBytes =
    4ULL * 1024ULL * 1024ULL;
inline constexpr std::uint32_t kExtensionPermissionLedgerMaxPersistedLeases =
    4096;
inline constexpr std::size_t kExtensionPermissionLedgerMaxProfileBytes = 256;
inline constexpr std::uint16_t kExtensionPermissionLedgerMaxWebsitesPerLease =
    256;
inline constexpr std::array<std::uint8_t, 4>
    kExtensionPermissionLedgerSnapshotMagic = {
        static_cast<std::uint8_t>('G'), static_cast<std::uint8_t>('C'),
        static_cast<std::uint8_t>('P'), static_cast<std::uint8_t>('L')};

enum class ExtensionPermissionLedgerStorageIssue {
  invalid_profile,
  snapshot_too_large,
  invalid_magic,
  unsupported_version,
  invalid_reserved_header,
  truncated_snapshot,
  checksum_mismatch,
  profile_mismatch,
  invalid_lease_count,
  duplicate_lease,
  invalid_enum_value,
  invalid_flags,
  invalid_lease,
  too_many_websites,
  invalid_website,
  duplicate_website,
  clock_regression,
  trailing_snapshot_bytes,
  io_error,
};

struct ExtensionPermissionLedgerSnapshotResult {
  std::vector<std::uint8_t> bytes;
  std::vector<ExtensionPermissionLedgerStorageIssue> issues;

  [[nodiscard]] bool accepted() const noexcept {
    return !bytes.empty() && issues.empty();
  }
};

struct ExtensionPermissionLedgerRestoreResult {
  ExtensionPermissionLedger ledger;
  std::vector<ExtensionPermissionLedgerStorageIssue> issues;
  std::size_t expired_leases_dropped{0};
  bool recovered_from_backup{false};

  [[nodiscard]] bool accepted() const noexcept { return issues.empty(); }
};

inline void add_extension_permission_ledger_storage_issue_once(
    std::vector<ExtensionPermissionLedgerStorageIssue>& issues,
    ExtensionPermissionLedgerStorageIssue issue) {
  if (std::find(issues.begin(), issues.end(), issue) == issues.end()) {
    issues.push_back(issue);
  }
}

inline bool valid_extension_permission_ledger_profile_id(
    std::string_view profile_id) {
  if (profile_id.empty() ||
      profile_id.size() > kExtensionPermissionLedgerMaxProfileBytes) {
    return false;
  }
  return std::none_of(profile_id.begin(), profile_id.end(),
                      [](unsigned char character) {
                        return character < 0x20U || character == 0x7fU;
                      });
}

inline std::uint32_t extension_permission_ledger_crc32(
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

class ExtensionPermissionLedgerByteWriter {
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

class ExtensionPermissionLedgerByteReader {
 public:
  explicit ExtensionPermissionLedgerByteReader(
      std::span<const std::uint8_t> bytes)
      : bytes_(bytes) {}

  [[nodiscard]] std::size_t remaining() const noexcept {
    return bytes_.size() - offset_;
  }

  [[nodiscard]] bool at_end() const noexcept {
    return offset_ == bytes_.size();
  }

  bool read_u8(std::uint8_t& value) {
    if (remaining() < 1) {
      return false;
    }
    value = bytes_[offset_++];
    return true;
  }

  bool read_u16(std::uint16_t& value) {
    if (remaining() < 2) {
      return false;
    }
    value = static_cast<std::uint16_t>(bytes_[offset_]) |
            (static_cast<std::uint16_t>(bytes_[offset_ + 1]) << 8U);
    offset_ += 2;
    return true;
  }

  bool read_u32(std::uint32_t& value) {
    if (remaining() < 4) {
      return false;
    }
    value = static_cast<std::uint32_t>(bytes_[offset_]) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 1]) << 8U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 2]) << 16U) |
            (static_cast<std::uint32_t>(bytes_[offset_ + 3]) << 24U);
    offset_ += 4;
    return true;
  }

  bool read_u64(std::uint64_t& value) {
    if (remaining() < 8) {
      return false;
    }
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
    if (length > remaining()) {
      return false;
    }
    value = bytes_.subspan(offset_, length);
    offset_ += length;
    return true;
  }

 private:
  std::span<const std::uint8_t> bytes_;
  std::size_t offset_{0};
};

inline std::uint8_t extension_permission_storage_code(
    ExtensionPermission permission) {
  return static_cast<std::uint8_t>(permission) + 1U;
}

inline std::optional<ExtensionPermission> parse_extension_permission_storage_code(
    std::uint8_t code) {
  if (code < 1U || code > 15U) {
    return std::nullopt;
  }
  return static_cast<ExtensionPermission>(code - 1U);
}

inline std::uint8_t extension_grant_scope_storage_code(
    ExtensionGrantScope scope) {
  return static_cast<std::uint8_t>(scope) + 1U;
}

inline std::optional<ExtensionGrantScope> parse_extension_grant_scope_storage_code(
    std::uint8_t code) {
  if (code < 1U || code > 5U) {
    return std::nullopt;
  }
  return static_cast<ExtensionGrantScope>(code - 1U);
}

inline std::uint8_t extension_grant_lifetime_storage_code(
    ExtensionGrantLifetime lifetime) {
  if (lifetime == ExtensionGrantLifetime::one_hour) {
    return 1U;
  }
  if (lifetime == ExtensionGrantLifetime::always) {
    return 2U;
  }
  return 0U;
}

inline std::optional<ExtensionGrantLifetime>
parse_extension_grant_lifetime_storage_code(std::uint8_t code) {
  if (code == 1U) {
    return ExtensionGrantLifetime::one_hour;
  }
  if (code == 2U) {
    return ExtensionGrantLifetime::always;
  }
  return std::nullopt;
}

inline bool extension_permission_lease_has_duplicate_websites(
    const ExtensionPermissionLease& lease) {
  std::unordered_set<std::string> seen;
  for (const auto& website : lease.grant.websites) {
    if (!seen.insert(website).second) {
      return true;
    }
  }
  return false;
}

inline bool extension_permission_lease_is_durable_candidate(
    const ExtensionPermissionLease& lease,
    std::string_view profile_id,
    std::uint64_t now_millis) {
  if (!lease.grant.active || lease.revoked || lease.consumed ||
      lease.grant.profile_id != profile_id) {
    return false;
  }
  if (lease.grant.lifetime != ExtensionGrantLifetime::one_hour &&
      lease.grant.lifetime != ExtensionGrantLifetime::always) {
    return false;
  }
  return lease.grant.lifetime != ExtensionGrantLifetime::one_hour ||
         now_millis < lease.expires_at_millis;
}

inline ExtensionPermissionLedgerSnapshotResult
encode_extension_permission_ledger_snapshot(
    const ExtensionPermissionLedger& ledger,
    std::string_view profile_id,
    std::uint64_t now_millis) {
  ExtensionPermissionLedgerSnapshotResult result;
  if (!valid_extension_permission_ledger_profile_id(profile_id)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::invalid_profile);
    return result;
  }

  std::vector<const ExtensionPermissionLease*> durable;
  durable.reserve(ledger.leases().size());
  for (const auto& lease : ledger.leases()) {
    if (!extension_permission_lease_is_durable_candidate(
            lease, profile_id, now_millis)) {
      continue;
    }
    if (!valid_extension_permission_lease(lease) ||
        !lease.tab_id.empty() || !lease.browser_session_id.empty() ||
        extension_permission_lease_has_duplicate_websites(lease)) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues, ExtensionPermissionLedgerStorageIssue::invalid_lease);
      return result;
    }
    if (lease.grant.lifetime == ExtensionGrantLifetime::one_hour &&
        now_millis < lease.issued_at_millis) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::clock_regression);
      return result;
    }
    if (lease.grant.websites.size() >
        kExtensionPermissionLedgerMaxWebsitesPerLease) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::too_many_websites);
      return result;
    }
    durable.push_back(&lease);
  }

  if (durable.size() > kExtensionPermissionLedgerMaxPersistedLeases) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::invalid_lease_count);
    return result;
  }

  ExtensionPermissionLedgerByteWriter writer;
  writer.write_bytes(kExtensionPermissionLedgerSnapshotMagic);
  writer.write_u8(kExtensionPermissionLedgerSnapshotVersion);
  writer.write_u8(0U);
  writer.write_u8(0U);
  writer.write_u8(0U);
  writer.write_u64(now_millis);
  if (!writer.write_string_u16(profile_id,
                               kExtensionPermissionLedgerMaxProfileBytes)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::invalid_profile);
    return result;
  }
  writer.write_u32(static_cast<std::uint32_t>(durable.size()));

  for (const auto* lease : durable) {
    writer.write_u64(lease->id);
    writer.write_u8(extension_permission_storage_code(lease->grant.permission));
    writer.write_u8(extension_grant_scope_storage_code(lease->grant.scope));
    writer.write_u8(
        extension_grant_lifetime_storage_code(lease->grant.lifetime));
    writer.write_u8(lease->grant.private_browsing_allowed ? 1U : 0U);
    writer.write_u64(lease->issued_at_millis);
    writer.write_u64(lease->expires_at_millis);
    writer.write_u16(
        static_cast<std::uint16_t>(lease->grant.websites.size()));
    for (const auto& website : lease->grant.websites) {
      if (!valid_extension_site_declaration(website) ||
          !writer.write_string_u16(website, 2048)) {
        add_extension_permission_ledger_storage_issue_once(
            result.issues,
            ExtensionPermissionLedgerStorageIssue::invalid_website);
        return result;
      }
    }
  }

  if (writer.bytes().size() + sizeof(std::uint32_t) >
      kExtensionPermissionLedgerMaxSnapshotBytes) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::snapshot_too_large);
    return result;
  }

  const auto checksum = extension_permission_ledger_crc32(writer.bytes());
  writer.write_u32(checksum);
  result.bytes = writer.take();
  return result;
}

inline ExtensionPermissionLedgerRestoreResult
restore_extension_permission_ledger_snapshot(
    std::span<const std::uint8_t> bytes,
    std::string_view expected_profile_id,
    std::uint64_t now_millis) {
  ExtensionPermissionLedgerRestoreResult result;
  if (!valid_extension_permission_ledger_profile_id(expected_profile_id)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::invalid_profile);
    return result;
  }
  if (bytes.size() > kExtensionPermissionLedgerMaxSnapshotBytes) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::snapshot_too_large);
    return result;
  }
  constexpr std::size_t kMinimumSnapshotBytes = 4U + 1U + 3U + 8U + 2U +
                                                4U + 4U;
  if (bytes.size() < kMinimumSnapshotBytes) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
    return result;
  }

  const auto payload_size = bytes.size() - sizeof(std::uint32_t);
  const auto payload = bytes.first(payload_size);
  ExtensionPermissionLedgerByteReader checksum_reader(
      bytes.subspan(payload_size));
  std::uint32_t stored_checksum = 0;
  if (!checksum_reader.read_u32(stored_checksum) ||
      stored_checksum != extension_permission_ledger_crc32(payload)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::checksum_mismatch);
    return result;
  }

  ExtensionPermissionLedgerByteReader reader(payload);
  std::span<const std::uint8_t> magic;
  if (!reader.read_bytes(kExtensionPermissionLedgerSnapshotMagic.size(),
                         magic)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
    return result;
  }
  if (!std::equal(magic.begin(), magic.end(),
                  kExtensionPermissionLedgerSnapshotMagic.begin())) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::invalid_magic);
    return result;
  }

  std::uint8_t version = 0;
  if (!reader.read_u8(version)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
    return result;
  }
  if (version != kExtensionPermissionLedgerSnapshotVersion) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::unsupported_version);
    return result;
  }
  for (int index = 0; index < 3; ++index) {
    std::uint8_t reserved = 0;
    if (!reader.read_u8(reserved)) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
      return result;
    }
    if (reserved != 0U) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::invalid_reserved_header);
      return result;
    }
  }

  std::uint64_t snapshot_time = 0;
  if (!reader.read_u64(snapshot_time)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
    return result;
  }

  std::string profile_id;
  if (!reader.read_string_u16(profile_id,
                              kExtensionPermissionLedgerMaxProfileBytes) ||
      !valid_extension_permission_ledger_profile_id(profile_id)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::invalid_profile);
    return result;
  }
  if (profile_id != expected_profile_id) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::profile_mismatch);
    return result;
  }

  std::uint32_t lease_count = 0;
  if (!reader.read_u32(lease_count)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
    return result;
  }
  if (lease_count > kExtensionPermissionLedgerMaxPersistedLeases) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::invalid_lease_count);
    return result;
  }

  std::unordered_set<std::uint64_t> seen_ids;
  for (std::uint32_t index = 0; index < lease_count; ++index) {
    std::uint64_t id = 0;
    std::uint8_t permission_code = 0;
    std::uint8_t scope_code = 0;
    std::uint8_t lifetime_code = 0;
    std::uint8_t flags = 0;
    std::uint64_t issued_at_millis = 0;
    std::uint64_t expires_at_millis = 0;
    std::uint16_t website_count = 0;
    if (!reader.read_u64(id) || !reader.read_u8(permission_code) ||
        !reader.read_u8(scope_code) || !reader.read_u8(lifetime_code) ||
        !reader.read_u8(flags) || !reader.read_u64(issued_at_millis) ||
        !reader.read_u64(expires_at_millis) ||
        !reader.read_u16(website_count)) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::truncated_snapshot);
      return result;
    }
    if (!seen_ids.insert(id).second) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::duplicate_lease);
      return result;
    }

    const auto permission =
        parse_extension_permission_storage_code(permission_code);
    const auto scope = parse_extension_grant_scope_storage_code(scope_code);
    const auto lifetime =
        parse_extension_grant_lifetime_storage_code(lifetime_code);
    if (!permission.has_value() || !scope.has_value() ||
        !lifetime.has_value()) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::invalid_enum_value);
      return result;
    }
    if ((flags & ~1U) != 0U) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::invalid_flags);
      return result;
    }
    if (website_count > kExtensionPermissionLedgerMaxWebsitesPerLease) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::too_many_websites);
      return result;
    }

    std::vector<std::string> websites;
    websites.reserve(website_count);
    std::unordered_set<std::string> seen_websites;
    for (std::uint16_t website_index = 0; website_index < website_count;
         ++website_index) {
      std::string website;
      if (!reader.read_string_u16(website, 2048) ||
          !valid_extension_site_declaration(website)) {
        add_extension_permission_ledger_storage_issue_once(
            result.issues,
            ExtensionPermissionLedgerStorageIssue::invalid_website);
        return result;
      }
      if (!seen_websites.insert(website).second) {
        add_extension_permission_ledger_storage_issue_once(
            result.issues,
            ExtensionPermissionLedgerStorageIssue::duplicate_website);
        return result;
      }
      websites.push_back(std::move(website));
    }

    ExtensionPermissionLease lease{
        .id = id,
        .grant = ExtensionPermissionGrant{
            .permission = *permission,
            .profile_id = profile_id,
            .scope = *scope,
            .lifetime = *lifetime,
            .websites = std::move(websites),
            .private_browsing_allowed = (flags & 1U) != 0U,
            .active = true,
        },
        .issued_at_millis = issued_at_millis,
        .expires_at_millis = expires_at_millis,
        .tab_id = {},
        .browser_session_id = {},
        .revoked = false,
        .consumed = false,
    };

    if (!valid_extension_permission_lease(lease)) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::invalid_lease);
      return result;
    }
    if (lease.grant.lifetime == ExtensionGrantLifetime::one_hour) {
      if (now_millis < lease.issued_at_millis) {
        add_extension_permission_ledger_storage_issue_once(
            result.issues,
            ExtensionPermissionLedgerStorageIssue::clock_regression);
        return result;
      }
      if (now_millis >= lease.expires_at_millis) {
        ++result.expired_leases_dropped;
        continue;
      }
    }
    if (!result.ledger.add(std::move(lease))) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues,
          ExtensionPermissionLedgerStorageIssue::invalid_lease);
      return result;
    }
  }

  if (!reader.at_end()) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues,
        ExtensionPermissionLedgerStorageIssue::trailing_snapshot_bytes);
    return result;
  }

  (void)snapshot_time;
  return result;
}

inline std::filesystem::path extension_permission_ledger_temp_path(
    const std::filesystem::path& path) {
  auto result = path;
  result += ".tmp";
  return result;
}

inline std::filesystem::path extension_permission_ledger_backup_path(
    const std::filesystem::path& path) {
  auto result = path;
  result += ".bak";
  return result;
}

inline bool read_extension_permission_ledger_snapshot_file(
    const std::filesystem::path& path,
    std::vector<std::uint8_t>& bytes) {
  std::error_code error;
  const auto file_size = std::filesystem::file_size(path, error);
  if (error || file_size > kExtensionPermissionLedgerMaxSnapshotBytes ||
      file_size > static_cast<std::uintmax_t>(
                      std::numeric_limits<std::size_t>::max())) {
    return false;
  }
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return false;
  }
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

inline bool write_extension_permission_ledger_snapshot_file(
    const std::filesystem::path& path,
    std::span<const std::uint8_t> bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    return false;
  }
  if (!bytes.empty()) {
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
  }
  output.flush();
  return static_cast<bool>(output);
}

inline std::vector<ExtensionPermissionLedgerStorageIssue>
save_extension_permission_ledger(
    const std::filesystem::path& path,
    const ExtensionPermissionLedger& ledger,
    std::string_view profile_id,
    std::uint64_t now_millis) {
  auto snapshot = encode_extension_permission_ledger_snapshot(
      ledger, profile_id, now_millis);
  if (!snapshot.accepted()) {
    return snapshot.issues;
  }

  const auto temp_path = extension_permission_ledger_temp_path(path);
  const auto backup_path = extension_permission_ledger_backup_path(path);
  std::error_code error;
  std::filesystem::remove(temp_path, error);
  error.clear();
  if (!write_extension_permission_ledger_snapshot_file(temp_path,
                                                       snapshot.bytes)) {
    std::filesystem::remove(temp_path, error);
    return {ExtensionPermissionLedgerStorageIssue::io_error};
  }

  const bool primary_exists = std::filesystem::exists(path, error) && !error;
  bool primary_valid = false;
  if (primary_exists) {
    std::vector<std::uint8_t> existing;
    if (read_extension_permission_ledger_snapshot_file(path, existing)) {
      primary_valid = restore_extension_permission_ledger_snapshot(
                          existing, profile_id, now_millis)
                          .accepted();
    }
  }
  error.clear();

  if (primary_valid) {
    std::filesystem::remove(backup_path, error);
    error.clear();
    std::filesystem::rename(path, backup_path, error);
    if (error) {
      std::filesystem::remove(temp_path, error);
      return {ExtensionPermissionLedgerStorageIssue::io_error};
    }
  } else if (primary_exists) {
    std::filesystem::remove(path, error);
    if (error) {
      std::filesystem::remove(temp_path, error);
      return {ExtensionPermissionLedgerStorageIssue::io_error};
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
    return {ExtensionPermissionLedgerStorageIssue::io_error};
  }
  return {};
}

inline ExtensionPermissionLedgerRestoreResult load_extension_permission_ledger(
    const std::filesystem::path& path,
    std::string_view expected_profile_id,
    std::uint64_t now_millis) {
  ExtensionPermissionLedgerRestoreResult result;
  if (!valid_extension_permission_ledger_profile_id(expected_profile_id)) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::invalid_profile);
    return result;
  }

  std::error_code error;
  const bool primary_exists = std::filesystem::exists(path, error);
  if (error) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::io_error);
    return result;
  }
  const auto backup_path = extension_permission_ledger_backup_path(path);
  const bool backup_exists = std::filesystem::exists(backup_path, error);
  if (error) {
    add_extension_permission_ledger_storage_issue_once(
        result.issues, ExtensionPermissionLedgerStorageIssue::io_error);
    return result;
  }

  // A profile that has never persisted extension grants starts with an empty,
  // valid ledger. Absence is not corruption and must not manufacture authority.
  if (!primary_exists && !backup_exists) {
    return result;
  }

  std::optional<ExtensionPermissionLedgerRestoreResult> primary_failure;
  if (primary_exists) {
    std::vector<std::uint8_t> bytes;
    if (!read_extension_permission_ledger_snapshot_file(path, bytes)) {
      ExtensionPermissionLedgerRestoreResult failed;
      add_extension_permission_ledger_storage_issue_once(
          failed.issues, ExtensionPermissionLedgerStorageIssue::io_error);
      primary_failure = std::move(failed);
    } else {
      auto primary = restore_extension_permission_ledger_snapshot(
          bytes, expected_profile_id, now_millis);
      if (primary.accepted()) {
        return primary;
      }
      primary_failure = std::move(primary);
    }
  }

  if (backup_exists) {
    std::vector<std::uint8_t> bytes;
    if (read_extension_permission_ledger_snapshot_file(backup_path, bytes)) {
      auto backup = restore_extension_permission_ledger_snapshot(
          bytes, expected_profile_id, now_millis);
      if (backup.accepted()) {
        backup.recovered_from_backup = true;
        return backup;
      }
      if (!primary_failure.has_value()) {
        return backup;
      }
    } else if (!primary_failure.has_value()) {
      add_extension_permission_ledger_storage_issue_once(
          result.issues, ExtensionPermissionLedgerStorageIssue::io_error);
      return result;
    }
  }

  if (primary_failure.has_value()) {
    return std::move(*primary_failure);
  }
  add_extension_permission_ledger_storage_issue_once(
      result.issues, ExtensionPermissionLedgerStorageIssue::io_error);
  return result;
}

}  // namespace goreecloud::browser
