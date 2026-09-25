#pragma once

#include <string>
#include <string_view>

namespace goreecloud::browser {

struct BrowserPanelPresentation {
  std::string eyebrow;
  std::string title;
  std::string body;
  std::string status;
};

inline BrowserPanelPresentation browser_panel_presentation(std::string_view payload) {
  if (payload == "bookmarks" || payload == "Unified Bookmarks") {
    return {
        "LIBRARY",
        "Bookmarks",
        "Local Bookmarks stay on this device. GoreeCloud Bookmarks remains a separate synchronized destination and is shown only when its accepted service adapter is available.",
        "Local and synchronized destinations remain separate",
    };
  }
  if (payload == "reader-mode" || payload == "Advanced Reader Mode") {
    return {
        "READING",
        "Reader Mode",
        "Reader Mode becomes available when the render-capable engine identifies supported readable page content. The Development renderer cannot make that determination.",
        "Render-capable page required",
    };
  }
  if (payload == "privacy-shield") {
    return {
        "PRIVACY SHIELD",
        "Privacy controls",
        "Browser privacy state is shown only from current authoritative Privacy Shield evidence. Missing or unaccepted runtime evidence must remain unverified.",
        "Runtime integration pending",
    };
  }
  if (payload == "wardveil-security" || payload == "Wardveil Security") {
    return {
        "WARDVEIL SECURITY",
        "Security",
        "Browser security state is shown only from current authoritative Wardveil evidence. A shield or icon never manufactures a protected state.",
        "Runtime integration pending",
    };
  }
  if (payload == "clipboard") {
    return {
        "BROWSER TOOLS",
        "Clipboard",
        "Clipboard actions will be exposed only for explicit supported Browser operations and page content made available by the render-capable engine.",
        "Renderer integration pending",
    };
  }
  if (payload == "dns-cache") {
    return {
        "NETWORK",
        "DNS cache",
        "DNS cache controls remain unavailable until the accepted DNS and network integration exposes the required Browser operation and evidence.",
        "DNS integration pending",
    };
  }
  if (payload == "proxy-manager") {
    return {
        "NETWORK",
        "Advanced Proxy Manager",
        "Proxy configuration remains a Browser-owned presentation surface backed by explicit network authority. No proxy state is inferred from UI configuration alone.",
        "Network integration pending",
    };
  }
  if (payload == "search-unavailable" || payload == "GoreeCloud Search unavailable") {
    return {
        "GOREECLOUD SEARCH",
        "Search unavailable",
        "GoreeCloud Search could not resolve this request. Direct structurally valid HTTP or HTTPS navigation remains independent from Search.",
        "No alternate search provider fallback",
    };
  }

  constexpr std::string_view kDownloadsPrefix = "Advanced Download Manager";
  if (payload.starts_with(kDownloadsPrefix)) {
    auto body = std::string{"No downloads."};
    if (const auto newline = payload.find('\n'); newline != std::string_view::npos) {
      body = std::string{payload.substr(newline + 1)};
      if (body.empty()) body = "No downloads.";
    }
    return {
        "DOWNLOADS",
        "Downloads",
        std::move(body),
        "Development download runtime",
    };
  }

  auto title = std::string{"Browser tool"};
  auto body = std::string{payload};
  if (const auto newline = payload.find('\n'); newline != std::string_view::npos) {
    title = std::string{payload.substr(0, newline)};
    body = std::string{payload.substr(newline + 1)};
  } else if (!payload.empty()) {
    title = std::string{payload};
    body = "This Browser-owned surface is still under development.";
  }
  return {"GOREECLOUD BROWSER", std::move(title), std::move(body),
          "Development surface"};
}

}  // namespace goreecloud::browser
