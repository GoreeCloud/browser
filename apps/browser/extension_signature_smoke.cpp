#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <sodium.h>

#include "goreecloud/browser/extension_signature.hpp"

using goreecloud::browser::ExtensionPackageFile;
using goreecloud::browser::ExtensionSignatureIssue;
using goreecloud::browser::ExtensionTrustState;

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

using Entry = std::pair<std::string, std::vector<std::uint8_t>>;

std::vector<std::uint8_t> bytes(std::string value) {
  return std::vector<std::uint8_t>(value.begin(), value.end());
}

std::string sample_manifest() {
  return "manifest_version=1\n"
         "api_version=1\n"
         "id=org.example.reader-helper\n"
         "name=Reader Helper\n"
         "version=1.0.0\n"
         "entry_point=scripts/background.js\n"
         "permission=page.read\n"
         "allowed_website=https://example.org\n";
}

std::vector<std::uint8_t> build_package(const std::vector<Entry>& entries) {
  std::vector<std::uint8_t> package;
  package.insert(package.end(),
                 goreecloud::browser::kNativeExtensionArchiveMagic.begin(),
                 goreecloud::browser::kNativeExtensionArchiveMagic.end());
  package.push_back(
      goreecloud::browser::kNativeExtensionPackageFormatVersion);
  package.insert(package.end(), 3, 0);
  append_u32(package, static_cast<std::uint32_t>(entries.size()));
  for (const auto& [path, contents] : entries) {
    append_u16(package, static_cast<std::uint16_t>(path.size()));
    append_u64(package, static_cast<std::uint64_t>(contents.size()));
    package.insert(package.end(), path.begin(), path.end());
    package.insert(package.end(), contents.begin(), contents.end());
  }
  return package;
}

template <typename Issue>
bool has_issue(const std::vector<Issue>& issues, Issue expected) {
  return std::find(issues.begin(), issues.end(), expected) != issues.end();
}

std::vector<Entry> unsigned_entries(std::string script = "console.log('ok');") {
  return {
      {"manifest", bytes(sample_manifest())},
      {"scripts/background.js", bytes(std::move(script))},
  };
}

}  // namespace

int main() {
  assert(sodium_init() >= 0);

  auto unsigned_package = goreecloud::browser::decode_native_extension_package(
      "reader-helper.gcex", build_package(unsigned_entries()));
  assert(unsigned_package.accepted());
  assert(unsigned_package.package->archive.inventory.trust_state ==
         ExtensionTrustState::unsigned_package);

  const auto unsigned_verification =
      goreecloud::browser::verify_native_extension_package_signature(
          *unsigned_package.package);
  assert(unsigned_verification.accepted());
  assert(!unsigned_verification.signature_present);
  assert(!unsigned_verification.signature_valid);
  assert(unsigned_verification.trust_state ==
         ExtensionTrustState::unsigned_package);
  assert(unsigned_package.package->archive.inventory.trust_state ==
         ExtensionTrustState::unsigned_package);

  const auto digest =
      goreecloud::browser::compute_native_extension_signature_digest(
          *unsigned_package.package);
  assert(digest.accepted());

  std::array<unsigned char, crypto_sign_SEEDBYTES> seed{};
  for (std::size_t index = 0; index < seed.size(); ++index) {
    seed[index] = static_cast<unsigned char>(index + 1U);
  }
  std::array<unsigned char, crypto_sign_PUBLICKEYBYTES> public_key{};
  std::array<unsigned char, crypto_sign_SECRETKEYBYTES> secret_key{};
  assert(crypto_sign_seed_keypair(public_key.data(), secret_key.data(),
                                  seed.data()) == 0);

  std::array<unsigned char, crypto_sign_BYTES> signature{};
  unsigned long long signature_bytes = 0;
  assert(crypto_sign_detached(signature.data(), &signature_bytes,
                              digest.digest.data(), digest.digest.size(),
                              secret_key.data()) == 0);
  assert(signature_bytes == signature.size());

  auto signed_entries = unsigned_entries();
  signed_entries.emplace_back(
      std::string(goreecloud::browser::kExtensionSignaturePublicKeyPath),
      std::vector<std::uint8_t>(public_key.begin(), public_key.end()));
  signed_entries.emplace_back(
      std::string(goreecloud::browser::kExtensionSignatureValuePath),
      std::vector<std::uint8_t>(signature.begin(), signature.end()));

  auto signed_package = goreecloud::browser::decode_native_extension_package(
      "reader-helper.gcex", build_package(signed_entries));
  assert(signed_package.accepted());
  assert(signed_package.package->archive.inventory.trust_state ==
         ExtensionTrustState::unsigned_package);

  const auto signed_verification =
      goreecloud::browser::verify_native_extension_package_signature(
          *signed_package.package);
  assert(signed_verification.accepted());
  assert(signed_verification.signature_present);
  assert(signed_verification.signature_valid);
  assert(signed_verification.trust_state ==
         ExtensionTrustState::signed_package);
  assert(signed_verification.public_key_fingerprint_sha256.size() == 64);
  assert(signed_package.package->archive.inventory.trust_state ==
         ExtensionTrustState::signed_package);
  assert(signed_package.package->archive.inventory.trust_state !=
         ExtensionTrustState::verified_signature);

  auto tampered_entries = unsigned_entries("console.log('tampered');");
  tampered_entries.emplace_back(
      std::string(goreecloud::browser::kExtensionSignaturePublicKeyPath),
      std::vector<std::uint8_t>(public_key.begin(), public_key.end()));
  tampered_entries.emplace_back(
      std::string(goreecloud::browser::kExtensionSignatureValuePath),
      std::vector<std::uint8_t>(signature.begin(), signature.end()));
  auto tampered_package = goreecloud::browser::decode_native_extension_package(
      "reader-helper.gcex", build_package(tampered_entries));
  assert(tampered_package.accepted());
  const auto tampered_verification =
      goreecloud::browser::verify_native_extension_package_signature(
          *tampered_package.package);
  assert(!tampered_verification.accepted());
  assert(has_issue(tampered_verification.issues,
                   ExtensionSignatureIssue::invalid_signature));
  assert(tampered_package.package->archive.inventory.trust_state ==
         ExtensionTrustState::unsigned_package);

  auto partial_entries = unsigned_entries();
  partial_entries.emplace_back(
      std::string(goreecloud::browser::kExtensionSignaturePublicKeyPath),
      std::vector<std::uint8_t>(public_key.begin(), public_key.end()));
  auto partial_package = goreecloud::browser::decode_native_extension_package(
      "reader-helper.gcex", build_package(partial_entries));
  assert(partial_package.accepted());
  const auto partial_verification =
      goreecloud::browser::verify_native_extension_package_signature(
          *partial_package.package);
  assert(!partial_verification.accepted());
  assert(has_issue(partial_verification.issues,
                   ExtensionSignatureIssue::incomplete_signature_material));

  auto unknown_signature_entries = unsigned_entries();
  unknown_signature_entries.emplace_back("signatures/unbound-metadata",
                                         bytes("not-bound"));
  auto unknown_signature_package =
      goreecloud::browser::decode_native_extension_package(
          "reader-helper.gcex", build_package(unknown_signature_entries));
  assert(unknown_signature_package.accepted());
  const auto unknown_signature_verification =
      goreecloud::browser::verify_native_extension_package_signature(
          *unknown_signature_package.package);
  assert(!unknown_signature_verification.accepted());
  assert(has_issue(
      unknown_signature_verification.issues,
      ExtensionSignatureIssue::malformed_signature_namespace));

  sodium_memzero(secret_key.data(), secret_key.size());
  sodium_memzero(seed.data(), seed.size());
  return 0;
}
