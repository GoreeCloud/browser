#include <iostream>
#include <string_view>

#include "goreecloud/browser/glaze_acceptance.hpp"

int main() {
  using namespace goreecloud::browser;

  static_assert(kCurrentGlazeUiStableVersion == std::string_view{"1.4.0"});
  static_assert(kCurrentGlazeUiStableRevision ==
                std::string_view{"84cb3db4884042f0fa25ed6d475a127fb110f596"});
  static_assert(kGlazeAcceptanceRequiredForProduction);
  static_assert(kGlazeHumanValidationDeferredToV141);
  static_assert(kBrowserGlazeCapabilities.optical_intelligence);
  static_assert(!kBrowserGlazeOpticalPolicy.page_content_may_drive_trusted_chrome);
  static_assert(!kBrowserGlazeOpticalPolicy.origin_identity_may_drive_trusted_chrome);
  static_assert(!kBrowserGlazeOpticalPolicy.favicon_color_may_drive_trusted_chrome);
  static_assert(!kBrowserGlazeOpticalPolicy.page_theme_color_may_drive_trusted_chrome);
  static_assert(!kBrowserGlazeOpticalPolicy.security_state_may_be_inferred_from_optics);
  static_assert(!kBrowserGlazeOpticalPolicy.privacy_state_may_be_inferred_from_optics);
  static_assert(kBrowserGlazeOpticalPolicy.trusted_chrome_memory_tint_percent == 0);

  if (kGlazeAcceptanceRequirements.empty()) {
    return 1;
  }

  std::cout << "GoreeCloud Browser GLAZE UI V1.4 native contract: PASS\n";
  return 0;
}
