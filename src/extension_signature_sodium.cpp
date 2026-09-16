#include "goreecloud/browser/extension_signature.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <vector>

#include <sodium.h>

namespace goreecloud::browser {
namespace {

void add_signature_issue_once(std::vector<ExtensionSignatureIssue>& issues,
                              ExtensionSignatureIssue issue) {
  if (std::find(issues.begin(), issues.end(), issue) == issues.end()) {
    issues.push_back(issue);
  }
}

bool signature_namespace_path(std::string_view path) {
  return path.starts_with(kExtensionSignatureNamespace);
}

bool sha256_update(crypto_hash_sha256_state& state,
                   std::span<const std::uint8_t> bytes) {
  return crypto_hash_sha256_update(state, bytes.data(), bytes.size()) == 0;
}

bool sha256_update_u8(crypto_hash_sha256_state& state, std::uint8_t value) {
  return sha256_update(state, std::span<const std::uint8_t>(&value, 1));
}

bool sha256_update_u16(crypto_hash_sha256_state& state, std::uint16_t value) {
  const std::array<std::uint8_t, 2> bytes = {
      static_cast<std::uint8_t>(value & 0xffU),
      static_cast<std::uint8_t>((value >> 8U) & 0xffU)};
  return sha256_update(state, bytes);
}

bool sha256_update_u32(crypto_hash_sha256_state& state, std::uint32_t value) {
  std::array<std::uint8_t, 4> bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[index] =
        static_cast<std::uint8_t>((value >> (index * 8U)) & 0xffU);
  }
  return sha256_update(state, bytes);
}

bool sha256_update_u64(crypto_hash_sha256_state& state, std::uint64_t value) {
  std::array<std::uint8_t, 8> bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[index] =
        static_cast<std::uint8_t>((value >> (index * 8U)) & 0xffU);
  }
  return sha256_update(state, bytes);
}

std::string lowercase_hex(std::span<const std::uint8_t> bytes) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string result;
  result.resize(bytes.size() * 2U);
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    result[index * 2U] = kHex[(bytes[index] >> 4U) & 0x0fU];
    result[index * 2U + 1U] = kHex[bytes[index] & 0x0fU];
  }
  return result;
}

}  // namespace

ExtensionPackageDigestResult compute_native_extension_signature_digest(
    const LoadedNativeExtensionPackage& package) {
  ExtensionPackageDigestResult result;
  if (sodium_init() < 0) {
    add_signature_issue_once(
        result.issues, ExtensionSignatureIssue::crypto_initialization_failed);
    return result;
  }

  std::vector<const ExtensionPackageFile*> covered_files;
  covered_files.reserve(package.archive.files.size());
  for (const auto& file : package.archive.files) {
    if (!signature_namespace_path(file.path)) {
      covered_files.push_back(&file);
    }
  }
  if (covered_files.size() > std::numeric_limits<std::uint32_t>::max()) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::digest_failed);
    return result;
  }
  std::sort(covered_files.begin(), covered_files.end(),
            [](const ExtensionPackageFile* left,
               const ExtensionPackageFile* right) {
              return left->path < right->path;
            });

  crypto_hash_sha256_state state{};
  if (crypto_hash_sha256_init(&state) != 0) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::digest_failed);
    return result;
  }

  const auto domain = std::span<const std::uint8_t>(
      reinterpret_cast<const std::uint8_t*>(
          kExtensionSignatureDigestDomain.data()),
      kExtensionSignatureDigestDomain.size());
  if (!sha256_update_u16(
          state, static_cast<std::uint16_t>(
                     kExtensionSignatureDigestDomain.size())) ||
      !sha256_update(state, domain) ||
      !sha256_update_u8(state, kNativeExtensionPackageFormatVersion) ||
      !sha256_update_u32(state,
                         static_cast<std::uint32_t>(covered_files.size()))) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::digest_failed);
    return result;
  }

  for (const auto* file : covered_files) {
    if (file->path.size() > std::numeric_limits<std::uint16_t>::max() ||
        file->contents.size() > std::numeric_limits<std::uint64_t>::max()) {
      add_signature_issue_once(result.issues,
                               ExtensionSignatureIssue::digest_failed);
      return result;
    }
    const auto path = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(file->path.data()),
        file->path.size());
    if (!sha256_update_u16(state,
                           static_cast<std::uint16_t>(file->path.size())) ||
        !sha256_update(state, path) ||
        !sha256_update_u64(
            state, static_cast<std::uint64_t>(file->contents.size())) ||
        !sha256_update(state, file->contents)) {
      add_signature_issue_once(result.issues,
                               ExtensionSignatureIssue::digest_failed);
      return result;
    }
  }

  if (crypto_hash_sha256_final(&state, result.digest.data()) != 0) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::digest_failed);
  }
  return result;
}

ExtensionSignatureVerificationResult verify_native_extension_package_signature(
    LoadedNativeExtensionPackage& package) {
  ExtensionSignatureVerificationResult result;
  result.trust_state = package.archive.inventory.trust_state;
  if (package.archive.inventory.trust_state !=
      ExtensionTrustState::unsigned_package) {
    add_signature_issue_once(
        result.issues,
        ExtensionSignatureIssue::unexpected_initial_trust_state);
    return result;
  }

  const ExtensionPackageFile* public_key_file = nullptr;
  const ExtensionPackageFile* signature_file = nullptr;
  for (const auto& file : package.archive.files) {
    if (file.path == kExtensionSignaturePublicKeyPath) {
      public_key_file = &file;
      result.signature_present = true;
    } else if (file.path == kExtensionSignatureValuePath) {
      signature_file = &file;
      result.signature_present = true;
    } else if (signature_namespace_path(file.path)) {
      result.signature_present = true;
      add_signature_issue_once(
          result.issues,
          ExtensionSignatureIssue::malformed_signature_namespace);
    }
  }

  if (!result.signature_present) {
    result.trust_state = ExtensionTrustState::unsigned_package;
    return result;
  }
  if (!result.issues.empty()) {
    return result;
  }
  if (public_key_file == nullptr || signature_file == nullptr) {
    add_signature_issue_once(
        result.issues,
        ExtensionSignatureIssue::incomplete_signature_material);
    return result;
  }
  if (public_key_file->contents.size() != kExtensionSignaturePublicKeyBytes) {
    add_signature_issue_once(
        result.issues, ExtensionSignatureIssue::invalid_public_key_size);
    return result;
  }
  if (signature_file->contents.size() != kExtensionSignatureValueBytes) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::invalid_signature_size);
    return result;
  }

  const auto digest = compute_native_extension_signature_digest(package);
  if (!digest.accepted()) {
    for (const auto issue : digest.issues) {
      add_signature_issue_once(result.issues, issue);
    }
    return result;
  }
  if (sodium_init() < 0) {
    add_signature_issue_once(
        result.issues, ExtensionSignatureIssue::crypto_initialization_failed);
    return result;
  }

  if (crypto_sign_verify_detached(signature_file->contents.data(),
                                  digest.digest.data(), digest.digest.size(),
                                  public_key_file->contents.data()) != 0) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::invalid_signature);
    return result;
  }

  std::array<std::uint8_t, crypto_hash_sha256_BYTES> fingerprint{};
  if (crypto_hash_sha256(fingerprint.data(), public_key_file->contents.data(),
                         public_key_file->contents.size()) != 0) {
    add_signature_issue_once(result.issues,
                             ExtensionSignatureIssue::digest_failed);
    return result;
  }

  result.signature_valid = true;
  result.trust_state = ExtensionTrustState::signed_package;
  result.public_key_fingerprint_sha256 = lowercase_hex(fingerprint);
  package.archive.inventory.trust_state = ExtensionTrustState::signed_package;
  return result;
}

}  // namespace goreecloud::browser
