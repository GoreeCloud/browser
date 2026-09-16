#pragma once

#include <algorithm>
#include <array>
#include <charconv>
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

#include "goreecloud/browser/native_extension.hpp"

namespace goreecloud::browser {

inline constexpr std::uint8_t kNativeExtensionPackageFormatVersion = 1;
inline constexpr std::uint64_t kNativeExtensionMaxPackageBytes =
    64ULL * 1024ULL * 1024ULL;
inline constexpr std::uint32_t kNativeExtensionMaxPackageEntries = 2048;
inline constexpr std::uint64_t kNativeExtensionMaxEntryBytes =
    16ULL * 1024ULL * 1024ULL;
inline constexpr std::uint64_t kNativeExtensionMaxTotalEntryBytes =
    64ULL * 1024ULL * 1024ULL;
inline constexpr std::uint64_t kNativeExtensionMaxManifestBytes =
    256ULL * 1024ULL;
inline constexpr std::size_t kNativeExtensionMaxManifestLineBytes =
    16ULL * 1024ULL;
inline constexpr std::array<std::uint8_t, 4> kNativeExtensionArchiveMagic = {
    static_cast<std::uint8_t>('G'), static_cast<std::uint8_t>('C'),
    static_cast<std::uint8_t>('E'), static_cast<std::uint8_t>('X')};

enum class ExtensionArchiveIssue {
  invalid_package_suffix,
  empty_archive,
  package_too_large,
  invalid_magic,
  unsupported_package_format_version,
  invalid_reserved_header,
  invalid_entry_count,
  truncated_archive,
  invalid_entry_path,
  duplicate_entry,
  entry_too_large,
  total_entry_bytes_too_large,
  manifest_too_large,
  missing_manifest,
  trailing_archive_bytes,
};

enum class ExtensionManifestDecodeIssue {
  empty_manifest,
  manifest_too_large,
  invalid_utf8,
  line_too_long,
  malformed_line,
  invalid_key,
  unknown_key,
  duplicate_scalar_key,
  invalid_integer,
  invalid_control_character,
  missing_required_key,
};

struct ExtensionPackageFile {
  std::string path;
  std::vector<std::uint8_t> contents;
};

struct NativeExtensionArchive {
  ExtensionPackageInventory inventory;
  std::vector<ExtensionPackageFile> files;
};

struct ExtensionArchiveDecodeResult {
  std::optional<NativeExtensionArchive> archive;
  std::vector<ExtensionArchiveIssue> issues;

  [[nodiscard]] bool accepted() const noexcept {
    return archive.has_value() && issues.empty();
  }
};

struct ExtensionManifestDecodeResult {
  ExtensionManifest manifest;
  std::vector<ExtensionManifestDecodeIssue> issues;

  [[nodiscard]] bool accepted() const noexcept { return issues.empty(); }
};

struct LoadedNativeExtensionPackage {
  NativeExtensionArchive archive;
  ExtensionManifest manifest;
};

struct NativeExtensionPackageLoadResult {
  std::optional<LoadedNativeExtensionPackage> package;
  std::vector<ExtensionArchiveIssue> archive_issues;
  std::vector<ExtensionManifestDecodeIssue> manifest_issues;
  ExtensionValidationResult validation;
  bool io_error{false};

  [[nodiscard]] bool accepted() const noexcept {
    return package.has_value() && archive_issues.empty() &&
           manifest_issues.empty() && validation.accepted() && !io_error;
  }
};

inline void add_extension_archive_issue_once(
    ExtensionArchiveDecodeResult& result,
    ExtensionArchiveIssue issue) {
  if (std::find(result.issues.begin(), result.issues.end(), issue) ==
      result.issues.end()) {
    result.issues.push_back(issue);
  }
}

inline void add_extension_manifest_decode_issue_once(
    ExtensionManifestDecodeResult& result,
    ExtensionManifestDecodeIssue issue) {
  if (std::find(result.issues.begin(), result.issues.end(), issue) ==
      result.issues.end()) {
    result.issues.push_back(issue);
  }
}

inline bool valid_extension_utf8(std::string_view value) {
  std::size_t i = 0;
  while (i < value.size()) {
    const auto first = static_cast<unsigned char>(value[i]);
    if (first <= 0x7f) {
      ++i;
      continue;
    }

    std::size_t count = 0;
    std::uint32_t code_point = 0;
    if (first >= 0xc2 && first <= 0xdf) {
      count = 2;
      code_point = first & 0x1fU;
    } else if (first >= 0xe0 && first <= 0xef) {
      count = 3;
      code_point = first & 0x0fU;
    } else if (first >= 0xf0 && first <= 0xf4) {
      count = 4;
      code_point = first & 0x07U;
    } else {
      return false;
    }
    if (i + count > value.size()) {
      return false;
    }
    for (std::size_t offset = 1; offset < count; ++offset) {
      const auto continuation =
          static_cast<unsigned char>(value[i + offset]);
      if ((continuation & 0xc0U) != 0x80U) {
        return false;
      }
      code_point = (code_point << 6U) | (continuation & 0x3fU);
    }
    if ((count == 3 && code_point < 0x800U) ||
        (count == 4 && code_point < 0x10000U) ||
        (code_point >= 0xd800U && code_point <= 0xdfffU) ||
        code_point > 0x10ffffU) {
      return false;
    }
    i += count;
  }
  return true;
}

inline bool extension_manifest_value_has_control(std::string_view value) {
  return std::any_of(value.begin(), value.end(), [](unsigned char byte) {
    return byte < 0x20U || byte == 0x7fU;
  });
}

class NativeExtensionByteReader {
 public:
  explicit NativeExtensionByteReader(std::span<const std::uint8_t> bytes)
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

inline ExtensionArchiveDecodeResult decode_native_extension_archive(
    std::string_view package_name,
    std::span<const std::uint8_t> bytes) {
  ExtensionArchiveDecodeResult result;

  if (!package_name.ends_with(kNativeExtensionPackageSuffix)) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::invalid_package_suffix);
  }
  if (bytes.empty()) {
    add_extension_archive_issue_once(result,
                                     ExtensionArchiveIssue::empty_archive);
    return result;
  }
  if (bytes.size() > kNativeExtensionMaxPackageBytes) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::package_too_large);
    return result;
  }

  NativeExtensionByteReader reader(bytes);
  std::span<const std::uint8_t> magic;
  if (!reader.read_bytes(kNativeExtensionArchiveMagic.size(), magic)) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::truncated_archive);
    return result;
  }
  if (!std::equal(magic.begin(), magic.end(),
                  kNativeExtensionArchiveMagic.begin())) {
    add_extension_archive_issue_once(result,
                                     ExtensionArchiveIssue::invalid_magic);
    return result;
  }

  std::uint8_t package_version = 0;
  if (!reader.read_u8(package_version)) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::truncated_archive);
    return result;
  }
  if (package_version != kNativeExtensionPackageFormatVersion) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::unsupported_package_format_version);
    return result;
  }

  for (int reserved_index = 0; reserved_index < 3; ++reserved_index) {
    std::uint8_t reserved = 0;
    if (!reader.read_u8(reserved)) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::truncated_archive);
      return result;
    }
    if (reserved != 0) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::invalid_reserved_header);
    }
  }

  std::uint32_t entry_count = 0;
  if (!reader.read_u32(entry_count)) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::truncated_archive);
    return result;
  }
  if (entry_count == 0 || entry_count > kNativeExtensionMaxPackageEntries) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::invalid_entry_count);
    return result;
  }

  NativeExtensionArchive archive;
  archive.inventory.package_name = std::string(package_name);
  archive.inventory.trust_state = ExtensionTrustState::unsigned_package;
  archive.inventory.entries.reserve(entry_count);
  archive.files.reserve(entry_count);

  std::unordered_set<std::string> seen_paths;
  std::uint64_t total_entry_bytes = 0;
  bool manifest_seen = false;

  for (std::uint32_t entry_index = 0; entry_index < entry_count;
       ++entry_index) {
    std::uint16_t path_length = 0;
    std::uint64_t content_length = 0;
    if (!reader.read_u16(path_length) ||
        !reader.read_u64(content_length)) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::truncated_archive);
      return result;
    }
    if (path_length == 0 || path_length > 1024) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::invalid_entry_path);
      return result;
    }
    if (content_length > kNativeExtensionMaxEntryBytes) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::entry_too_large);
      return result;
    }
    if (total_entry_bytes >
        kNativeExtensionMaxTotalEntryBytes - content_length) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::total_entry_bytes_too_large);
      return result;
    }

    std::span<const std::uint8_t> path_bytes;
    if (!reader.read_bytes(path_length, path_bytes)) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::truncated_archive);
      return result;
    }
    const std::string path(
        reinterpret_cast<const char*>(path_bytes.data()), path_bytes.size());
    if (!valid_extension_utf8(path) ||
        !valid_extension_package_path(path) || path.back() == '/') {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::invalid_entry_path);
      return result;
    }
    if (!seen_paths.insert(path).second) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::duplicate_entry);
      return result;
    }
    if (path == kNativeExtensionManifestPath &&
        content_length > kNativeExtensionMaxManifestBytes) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::manifest_too_large);
      return result;
    }

    if (content_length > static_cast<std::uint64_t>(
                             std::numeric_limits<std::size_t>::max())) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::entry_too_large);
      return result;
    }
    std::span<const std::uint8_t> contents;
    if (!reader.read_bytes(static_cast<std::size_t>(content_length),
                           contents)) {
      add_extension_archive_issue_once(
          result, ExtensionArchiveIssue::truncated_archive);
      return result;
    }

    total_entry_bytes += content_length;
    manifest_seen = manifest_seen || path == kNativeExtensionManifestPath;
    archive.inventory.entries.push_back(
        ExtensionPackageEntry{.path = path,
                              .uncompressed_bytes = content_length});
    archive.files.push_back(
        ExtensionPackageFile{.path = path,
                             .contents = std::vector<std::uint8_t>(
                                 contents.begin(), contents.end())});
  }

  if (!manifest_seen) {
    add_extension_archive_issue_once(result,
                                     ExtensionArchiveIssue::missing_manifest);
  }
  if (!reader.at_end()) {
    add_extension_archive_issue_once(
        result, ExtensionArchiveIssue::trailing_archive_bytes);
  }
  if (result.issues.empty()) {
    result.archive = std::move(archive);
  }
  return result;
}

inline bool valid_extension_manifest_key(std::string_view key) {
  if (key.empty()) {
    return false;
  }
  return std::all_of(key.begin(), key.end(),
                     [](unsigned char character) {
                       return (character >= 'a' && character <= 'z') ||
                              character == '_';
                     });
}

inline bool parse_native_extension_manifest_integer(std::string_view value,
                                                    int& target) {
  if (value.empty() ||
      !std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return character >= '0' && character <= '9';
      })) {
    return false;
  }
  int parsed = 0;
  const auto [end, error] =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (error != std::errc{} || end != value.data() + value.size()) {
    return false;
  }
  target = parsed;
  return true;
}

inline ExtensionManifestDecodeResult decode_native_extension_manifest(
    std::span<const std::uint8_t> bytes) {
  ExtensionManifestDecodeResult result;
  if (bytes.empty()) {
    add_extension_manifest_decode_issue_once(
        result, ExtensionManifestDecodeIssue::empty_manifest);
    return result;
  }
  if (bytes.size() > kNativeExtensionMaxManifestBytes) {
    add_extension_manifest_decode_issue_once(
        result, ExtensionManifestDecodeIssue::manifest_too_large);
    return result;
  }

  const std::string_view text(
      reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (!valid_extension_utf8(text)) {
    add_extension_manifest_decode_issue_once(
        result, ExtensionManifestDecodeIssue::invalid_utf8);
    return result;
  }

  bool manifest_version_seen = false;
  bool api_version_seen = false;
  bool id_seen = false;
  bool name_seen = false;
  bool version_seen = false;
  bool description_seen = false;
  bool author_seen = false;
  bool homepage_seen = false;
  bool minimum_browser_version_seen = false;

  auto reject_duplicate = [&](bool& seen) {
    if (seen) {
      add_extension_manifest_decode_issue_once(
          result, ExtensionManifestDecodeIssue::duplicate_scalar_key);
      return true;
    }
    seen = true;
    return false;
  };

  std::size_t cursor = 0;
  while (cursor <= text.size()) {
    const auto newline = text.find('\n', cursor);
    const auto line_end =
        newline == std::string_view::npos ? text.size() : newline;
    auto line = text.substr(cursor, line_end - cursor);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }
    if (line.size() > kNativeExtensionMaxManifestLineBytes) {
      add_extension_manifest_decode_issue_once(
          result, ExtensionManifestDecodeIssue::line_too_long);
      return result;
    }
    if (!line.empty()) {
      const auto delimiter = line.find('=');
      if (delimiter == std::string_view::npos || delimiter == 0) {
        add_extension_manifest_decode_issue_once(
            result, ExtensionManifestDecodeIssue::malformed_line);
        return result;
      }
      const auto key = line.substr(0, delimiter);
      const auto value = line.substr(delimiter + 1);
      if (!valid_extension_manifest_key(key)) {
        add_extension_manifest_decode_issue_once(
            result, ExtensionManifestDecodeIssue::invalid_key);
        return result;
      }
      if (extension_manifest_value_has_control(value)) {
        add_extension_manifest_decode_issue_once(
            result,
            ExtensionManifestDecodeIssue::invalid_control_character);
        return result;
      }

      if (key == "manifest_version") {
        if (reject_duplicate(manifest_version_seen)) {
          return result;
        }
        if (!parse_native_extension_manifest_integer(
                value, result.manifest.manifest_version)) {
          add_extension_manifest_decode_issue_once(
              result, ExtensionManifestDecodeIssue::invalid_integer);
          return result;
        }
      } else if (key == "api_version") {
        if (reject_duplicate(api_version_seen)) {
          return result;
        }
        if (!parse_native_extension_manifest_integer(
                value, result.manifest.api_version)) {
          add_extension_manifest_decode_issue_once(
              result, ExtensionManifestDecodeIssue::invalid_integer);
          return result;
        }
      } else if (key == "id") {
        if (reject_duplicate(id_seen)) {
          return result;
        }
        result.manifest.id = std::string(value);
      } else if (key == "name") {
        if (reject_duplicate(name_seen)) {
          return result;
        }
        result.manifest.name = std::string(value);
      } else if (key == "version") {
        if (reject_duplicate(version_seen)) {
          return result;
        }
        result.manifest.version = std::string(value);
      } else if (key == "description") {
        if (reject_duplicate(description_seen)) {
          return result;
        }
        result.manifest.description = std::string(value);
      } else if (key == "author") {
        if (reject_duplicate(author_seen)) {
          return result;
        }
        result.manifest.author = std::string(value);
      } else if (key == "homepage") {
        if (reject_duplicate(homepage_seen)) {
          return result;
        }
        result.manifest.homepage = std::string(value);
      } else if (key == "minimum_browser_version") {
        if (reject_duplicate(minimum_browser_version_seen)) {
          return result;
        }
        result.manifest.minimum_browser_version = std::string(value);
      } else if (key == "entry_point") {
        result.manifest.entry_points.emplace_back(value);
      } else if (key == "permission") {
        result.manifest.requested_permissions.emplace_back(value);
      } else if (key == "allowed_website") {
        result.manifest.allowed_websites.emplace_back(value);
      } else {
        add_extension_manifest_decode_issue_once(
            result, ExtensionManifestDecodeIssue::unknown_key);
        return result;
      }
    }

    if (newline == std::string_view::npos) {
      break;
    }
    cursor = newline + 1;
  }

  if (!manifest_version_seen || !api_version_seen || !id_seen ||
      !name_seen || !version_seen) {
    add_extension_manifest_decode_issue_once(
        result, ExtensionManifestDecodeIssue::missing_required_key);
  }
  return result;
}

inline NativeExtensionPackageLoadResult decode_native_extension_package(
    std::string_view package_name,
    std::span<const std::uint8_t> bytes) {
  NativeExtensionPackageLoadResult result;
  auto archive_result =
      decode_native_extension_archive(package_name, bytes);
  result.archive_issues = archive_result.issues;
  if (!archive_result.accepted()) {
    return result;
  }

  auto archive = std::move(*archive_result.archive);
  const auto manifest_file = std::find_if(
      archive.files.begin(), archive.files.end(),
      [](const ExtensionPackageFile& file) {
        return file.path == kNativeExtensionManifestPath;
      });
  if (manifest_file == archive.files.end()) {
    result.archive_issues.push_back(ExtensionArchiveIssue::missing_manifest);
    return result;
  }

  const auto manifest_result = decode_native_extension_manifest(
      std::span<const std::uint8_t>(manifest_file->contents.data(),
                                    manifest_file->contents.size()));
  result.manifest_issues = manifest_result.issues;
  if (!manifest_result.accepted()) {
    return result;
  }

  result.validation =
      validate_extension_package(archive.inventory, manifest_result.manifest);
  if (!result.validation.accepted()) {
    return result;
  }

  result.package = LoadedNativeExtensionPackage{
      .archive = std::move(archive),
      .manifest = manifest_result.manifest};
  return result;
}

inline NativeExtensionPackageLoadResult read_native_extension_package(
    const std::filesystem::path& path) {
  NativeExtensionPackageLoadResult result;
  std::error_code error;
  const auto file_size = std::filesystem::file_size(path, error);
  if (error) {
    result.io_error = true;
    return result;
  }
  if (file_size > kNativeExtensionMaxPackageBytes ||
      file_size > static_cast<std::uintmax_t>(
                      std::numeric_limits<std::size_t>::max())) {
    result.archive_issues.push_back(ExtensionArchiveIssue::package_too_large);
    return result;
  }

  std::ifstream input(path, std::ios::binary);
  if (!input) {
    result.io_error = true;
    return result;
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file_size));
  if (!bytes.empty()) {
    input.read(reinterpret_cast<char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    if (!input ||
        static_cast<std::size_t>(input.gcount()) != bytes.size()) {
      result.io_error = true;
      return result;
    }
  }

  return decode_native_extension_package(
      path.filename().string(),
      std::span<const std::uint8_t>(bytes.data(), bytes.size()));
}

}  // namespace goreecloud::browser