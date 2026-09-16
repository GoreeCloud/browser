#pragma once

#include <string>
#include <vector>

#include "goreecloud/browser/native_extension.hpp"

namespace goreecloud::browser {

enum class ExtensionSupportState {
  disabled,
  foundation_only,
  native_runtime,
};

struct ExtensionPolicy {
  ExtensionSupportState support{ExtensionSupportState::disabled};
  bool developer_mode_enabled{false};
  bool native_package_installation_enabled{false};
  bool external_compatibility_layer_exposed{false};
  bool centralized_store_exposed{false};
  bool private_browsing_default_enabled{false};
};

struct ExtensionDescriptor {
  std::string id;
  std::string name;
  std::vector<std::string> permissions;
};

class ExtensionManager {
 public:
  virtual ~ExtensionManager() = default;
  [[nodiscard]] virtual ExtensionPolicy policy() const = 0;
  [[nodiscard]] virtual std::vector<ExtensionDescriptor> installed() const = 0;
};

// The native manifest/package/authorization foundation is intentionally
// independent from extension execution. No installable runtime is enabled by
// these types, and external compatibility layers/centralized stores are not
// part of the GoreeCloud Browser extension model.

}  // namespace goreecloud::browser
