#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "goreecloud/browser/native_extension_package.hpp"

using goreecloud::browser::ExtensionArchiveIssue;
using goreecloud::browser::ExtensionManifestDecodeIssue;
using goreecloud::browser::ExtensionValidationIssue;

namespace {

void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
  bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
  for (int shift = 0; shift < 32; shift += 8) {
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
}

void append_u64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffU));
  }
}

using Entry = std::pair<std::string, std::string>;

std::string sample_manifest() {
  return "manifest_version=1\n"
         "api_version=1\n"
         "id=org.example.reader-helper\n"
         "name=Reader Helper\n"
         "version=1.0.0\n"
         "description=Test extension\n"
         "author=Example\n"
         "homepage=https://example.org\n"
         "minimum_browser_version=0.1.0\n"
         "entry_point=scripts/background.js\n"
         "entry_point=pages/panel.html\n"
         "permission=page.read\n"
         "permission=notifications.display\n"
         "allowed_website=https://example.org\n";
}

std::vector<std::uint8_t> build_package(const std::vector<Entry>& entries) {
  std::vector<std::uint8_t> bytes;
  bytes.insert(bytes.end(),
               goreecloud::browser::kNativeExtensionArchiveMagic.begin(),
               goreecloud::browser::kNativeExtensionArchiveMagic.end());
  bytes.push_back(goreecloud::browser::kNativeExtensionPackageFormatVersion);
  bytes.insert(bytes.end(), 3, 0);
  append_u32(bytes, static_cast<std::uint32_t>(entries.size()));
  for (const auto& [path, contents] : entries) {
    append_u16(bytes, static_cast<std::uint16_t>(path.size()));
    append_u64(bytes, static_cast<std::uint64_t>(contents.size()));
    bytes.insert(bytes.end(), path.begin(), path.end());
    bytes.insert(bytes.end(), contents.begin(), contents.end());
  }
  return bytes;
}

template <typename Issue>
bool has_issue(const std::vector<Issue>& issues, Issue expected) {
  return std::find(issues.begin(), issues.end(), expected) != issues.end();
}

}  // namespace

int main() {
  const std::vector<Entry> entries{
      {"manifest", sample_manifest()},
      {"scripts/background.js", "console.log('background');"},
      {"pages/panel.html", "<main>Panel</main>"},
  };
  const auto bytes = build_package(entries);
  const auto decoded = goreecloud::browser::decode_native_extension_package(
      "reader-helper.gcex", bytes);
  assert(decoded.accepted());
  assert(decoded.package->manifest.id == "org.example.reader-helper");
  assert(decoded.package->manifest.entry_points.size() == 2);
  assert(decoded.package->archive.files.size() == 3);

  const auto temp_path = std::filesystem::temp_directory_path() /
                         "goreecloud-browser-extension-package-smoke.gcex";
  {
    std::ofstream output(temp_path, std::ios::binary | std::ios::trunc);
    assert(output);
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    assert(output);
  }
  const auto from_disk =
      goreecloud::browser::read_native_extension_package(temp_path);
  std::error_code remove_error;
  std::filesystem::remove(temp_path, remove_error);
  assert(from_disk.accepted());
  assert(from_disk.package->manifest.name == "Reader Helper");

  const auto wrong_suffix =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.zip", bytes);
  assert(!wrong_suffix.accepted());
  assert(has_issue(wrong_suffix.archive_issues,
                   ExtensionArchiveIssue::invalid_package_suffix));

  auto invalid_magic = bytes;
  invalid_magic[0] = 'X';
  const auto invalid_magic_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", invalid_magic);
  assert(!invalid_magic_result.accepted());
  assert(has_issue(invalid_magic_result.archive_issues,
                   ExtensionArchiveIssue::invalid_magic));

  const auto traversing = build_package({
      {"manifest", sample_manifest()},
      {"../escape.js", "bad"},
      {"scripts/background.js", "ok"},
      {"pages/panel.html", "ok"},
  });
  const auto traversal_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", traversing);
  assert(!traversal_result.accepted());
  assert(has_issue(traversal_result.archive_issues,
                   ExtensionArchiveIssue::invalid_entry_path));

  const auto duplicate = build_package({
      {"manifest", sample_manifest()},
      {"scripts/background.js", "one"},
      {"scripts/background.js", "two"},
      {"pages/panel.html", "ok"},
  });
  const auto duplicate_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", duplicate);
  assert(!duplicate_result.accepted());
  assert(has_issue(duplicate_result.archive_issues,
                   ExtensionArchiveIssue::duplicate_entry));

  const auto missing_manifest = build_package({
      {"scripts/background.js", "one"},
      {"pages/panel.html", "two"},
  });
  const auto missing_manifest_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", missing_manifest);
  assert(!missing_manifest_result.accepted());
  assert(has_issue(missing_manifest_result.archive_issues,
                   ExtensionArchiveIssue::missing_manifest));

  const auto unknown_manifest_key = build_package({
      {"manifest", sample_manifest() + "unrestricted=true\n"},
      {"scripts/background.js", "one"},
      {"pages/panel.html", "two"},
  });
  const auto unknown_key_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", unknown_manifest_key);
  assert(!unknown_key_result.accepted());
  assert(has_issue(unknown_key_result.manifest_issues,
                   ExtensionManifestDecodeIssue::unknown_key));

  const auto duplicate_scalar = build_package({
      {"manifest", sample_manifest() + "id=org.example.other\n"},
      {"scripts/background.js", "one"},
      {"pages/panel.html", "two"},
  });
  const auto duplicate_scalar_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", duplicate_scalar);
  assert(!duplicate_scalar_result.accepted());
  assert(has_issue(duplicate_scalar_result.manifest_issues,
                   ExtensionManifestDecodeIssue::duplicate_scalar_key));

  const auto missing_required = build_package({
      {"manifest",
       "api_version=1\nid=org.example.reader-helper\nname=Reader Helper\n"
       "version=1.0.0\nentry_point=scripts/background.js\n"},
      {"scripts/background.js", "one"},
  });
  const auto missing_required_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", missing_required);
  assert(!missing_required_result.accepted());
  assert(has_issue(missing_required_result.manifest_issues,
                   ExtensionManifestDecodeIssue::missing_required_key));

  auto unsupported_manifest = sample_manifest();
  unsupported_manifest.replace(
      unsupported_manifest.find("manifest_version=1"),
      std::string("manifest_version=1").size(), "manifest_version=2");
  const auto unsupported = build_package({
      {"manifest", unsupported_manifest},
      {"scripts/background.js", "one"},
      {"pages/panel.html", "two"},
  });
  const auto unsupported_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", unsupported);
  assert(!unsupported_result.accepted());
  assert(has_issue(unsupported_result.validation.issues,
                   ExtensionValidationIssue::unsupported_manifest_version));

  const auto unknown_permission = build_package({
      {"manifest", sample_manifest() +
                       "permission=browser.unrestricted\n"},
      {"scripts/background.js", "one"},
      {"pages/panel.html", "two"},
  });
  const auto unknown_permission_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", unknown_permission);
  assert(!unknown_permission_result.accepted());
  assert(has_issue(unknown_permission_result.validation.issues,
                   ExtensionValidationIssue::unknown_permission));

  std::vector<std::uint8_t> oversized_entry;
  oversized_entry.insert(
      oversized_entry.end(),
      goreecloud::browser::kNativeExtensionArchiveMagic.begin(),
      goreecloud::browser::kNativeExtensionArchiveMagic.end());
  oversized_entry.push_back(
      goreecloud::browser::kNativeExtensionPackageFormatVersion);
  oversized_entry.insert(oversized_entry.end(), 3, 0);
  append_u32(oversized_entry, 1);
  append_u16(oversized_entry, 8);
  append_u64(oversized_entry,
             goreecloud::browser::kNativeExtensionMaxEntryBytes + 1);
  oversized_entry.insert(oversized_entry.end(),
                         {'m', 'a', 'n', 'i', 'f', 'e', 's', 't'});
  const auto oversized_entry_result =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", oversized_entry);
  assert(!oversized_entry_result.accepted());
  assert(has_issue(oversized_entry_result.archive_issues,
                   ExtensionArchiveIssue::entry_too_large));

  return 0;
}
