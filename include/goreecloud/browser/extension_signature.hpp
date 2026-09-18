#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "goreecloud/browser/native_extension_package.hpp"

namespace goreecloud::browser {

inline constexpr std::string_view kExtensionSignatureNamespace =
    "signatures/";
inline constexpr std::string_view kExtensionSignaturePublicKeyPath =
    "signatures/ed25519-sha256-v1.public-key";
inline constexpr std::string_view kExtensionSignatureValuePath =
    "signatures/ed25519-sha256-v1.signature";
inline constexpr std::string_view kExtensionSignatureScheme =
    "ed25519-sha256-v1";
inline constexpr std::string_view kExtensionSignatureDigestDomain =
    "GoreeCloud GCEX signature payload v1";
inline constexpr std::size_t kExtensionSignaturePublicKeyBytes = 32;
inline constexpr std::size_t kExtensionSignatureValueBytes = 64;
inline constexpr std::size_t kExtensionSignatureDigestBytes = 32;

enum class ExtensionSignatureIssue {
  crypto_initialization_failed,
  malformed_signature_namespace,
  incomplete_signature_material,
  invalid_public_key_size,
  invalid_signature_size,
  digest_failed,
  invalid_signature,
  unexpected_initial_trust_state,
};

struct ExtensionPackageDigestResult {
  std::array<std::uint8_t, kExtensionSignatureDigestBytes> digest{};
  std::vector<ExtensionSignatureIssue> issues;

  [[nodiscard]] bool accepted() const noexcept { return issues.empty(); }
};

struct ExtensionSignatureVerificationResult {
  bool signature_present{false};
  bool signature_valid{false};
  ExtensionTrustState trust_state{ExtensionTrustState::unsigned_package};
  std::string public_key_fingerprint_sha256;
  std::vector<ExtensionSignatureIssue> issues;

  [[nodiscard]] bool accepted() const noexcept { return issues.empty(); }
};

// Produces the deterministic SHA-256 digest used by the first GCEX signature
// scheme. Signature-namespace files are excluded; every other decoded package
// file is bound in lexicographic path order together with lengths and the
// package-format version.
ExtensionPackageDigestResult compute_native_extension_signature_digest(
    const LoadedNativeExtensionPackage& package);

// Verifies the optional package-local Ed25519 signature. Raw GCEX decoding
// must have left the package unsigned. A valid package-local signature can
// promote only to signed_package; verified_signature requires an independent
// developer-key trust authority and is deliberately not produced here.
ExtensionSignatureVerificationResult verify_native_extension_package_signature(
    LoadedNativeExtensionPackage& package);

}  // namespace goreecloud::browser
