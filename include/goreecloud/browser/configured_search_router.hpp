#pragma once

#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "goreecloud/browser/omnibox_controller.hpp"

namespace goreecloud::browser {

class ConfiguredGoreeCloudSearchRouter final : public GoreeCloudSearchRouter {
 public:
  explicit ConfiguredGoreeCloudSearchRouter(std::string endpoint)
      : endpoint_(std::move(endpoint)), valid_(is_valid_endpoint(endpoint_)) {}

  [[nodiscard]] std::string search_url(std::string_view query) const override {
    if (!valid_) {
      throw std::runtime_error(
          "GoreeCloud Search is not configured with a valid HTTP(S) endpoint; "
          "no alternate search provider is permitted");
    }

    std::string result = endpoint_;
    if (endpoint_.find('?') == std::string::npos) {
      result += "?q=";
    } else if (endpoint_.back() == '?' || endpoint_.back() == '&') {
      result += "q=";
    } else {
      result += "&q=";
    }
    result += percent_encode(query);
    return result;
  }

  [[nodiscard]] bool configured() const noexcept { return valid_; }

  [[nodiscard]] static bool is_valid_endpoint(std::string_view endpoint) noexcept {
    if (endpoint.empty() || endpoint.find('#') != std::string_view::npos) {
      return false;
    }

    for (const unsigned char ch : endpoint) {
      if (std::iscntrl(ch) || std::isspace(ch)) {
        return false;
      }
    }

    std::size_t authority_start = 0;
    if (endpoint.rfind("https://", 0) == 0) {
      authority_start = 8;
    } else if (endpoint.rfind("http://", 0) == 0) {
      authority_start = 7;
    } else {
      return false;
    }

    const auto authority_end = endpoint.find_first_of("/?", authority_start);
    const auto authority = endpoint.substr(
        authority_start,
        authority_end == std::string_view::npos ? std::string_view::npos
                                                : authority_end - authority_start);
    if (authority.empty() || authority.find('@') != std::string_view::npos) {
      return false;
    }

    return true;
  }

 private:
  [[nodiscard]] static std::string percent_encode(std::string_view value) {
    std::ostringstream out;
    out << std::uppercase << std::hex;
    for (const unsigned char ch : value) {
      if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
        out << static_cast<char>(ch);
      } else {
        out << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(ch);
      }
    }
    return out.str();
  }

  std::string endpoint_;
  bool valid_{false};
};

inline ConfiguredGoreeCloudSearchRouter search_router_from_environment() {
  const char* endpoint = std::getenv("GOREECLOUD_SEARCH_URL");
  return ConfiguredGoreeCloudSearchRouter(endpoint ? std::string{endpoint} : std::string{});
}

}  // namespace goreecloud::browser
