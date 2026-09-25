#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

#include "goreecloud/browser/advanced_download_panel.hpp"
#include "goreecloud/browser/application.hpp"
#include "goreecloud/browser/browser_media_action_backend.hpp"
#include "goreecloud/browser/chrome_command_router.hpp"
#include "goreecloud/browser/chrome_shell.hpp"
#include "goreecloud/browser/configured_search_router.hpp"
#include "goreecloud/browser/download_runtime_factory.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/media_action_executor.hpp"
#include "goreecloud/browser/media_destination_service.hpp"
#include "goreecloud/browser/media_preview_provider.hpp"
#include "goreecloud/browser/media_visual_search_router.hpp"
#include "goreecloud/browser/omnibox_controller.hpp"
#include "goreecloud/browser/platform/gtk_linux_glaze_host.hpp"

namespace goreecloud::browser::platform {

inline int run_gtk_linux_browser(BrowserApplication& application) {
  auto* window = application.first_window();
  if (!window || !window->active_tab()) {
    throw std::runtime_error("GoreeCloud Browser has no initial window/tab for GTK host");
  }

  GtkLinuxGlazeWindowHost host;
  host.set_private_window(window->private_window());
  BrowserChromeShell chrome(*window);
  ChromeCommandRouter commands(*window);
  auto search_router = search_router_from_environment();
  auto visual_search_router = visual_search_router_from_environment();
  OmniboxController omnibox(search_router);

  MediaHoverSitePolicy media_policy;
  media_policy.allow_remote_processing = true;
  host.set_media_hover_policy(media_policy);

  auto download_runtime = make_download_runtime();
  auto& downloads = download_runtime->service();
  auto* live_downloads = dynamic_cast<AdvancedDownloadRuntimeService*>(&downloads);
  UnavailableMediaDestinationService media_destinations;
  BrowserMediaActionBackend media_backend(
      visual_search_router,
      downloads,
      [&](std::string_view url) {
        commands.clear_panel();
        window->navigate_active(url);
        if (auto* tab = window->active_tab()) host.attach_engine_view(tab->engine_view());
      },
      [&](std::string_view url) {
        (void)window->new_tab(std::string{url});
      },
      [&](std::string_view text) { return host.copy_text_to_clipboard(text); },
      [&](const MediaTarget& target) {
        auto* tab = window->active_tab();
        if (!tab) return false;
        auto* provider = dynamic_cast<AsyncMediaPreviewProvider*>(&tab->engine_view());
        if (!provider) return false;
        MediaPreviewRequest preview_request;
        preview_request.target = target;
        preview_request.maximum_width = 960;
        preview_request.maximum_height = 720;
        preview_request.allow_animation = target.animated;
        return provider->request_media_preview(
            preview_request,
            [&](std::optional<MediaPreviewFrame> frame, std::string error) {
              if (!frame) {
                host.show_media_action_status(error.empty() ? "Media preview is unavailable." : error);
                return;
              }
              const std::string description = target.alt_text.empty() ? "Media preview" : target.alt_text;
              if (!host.show_media_preview(*frame, description)) {
                host.show_media_action_status("Media preview could not be displayed.");
              }
            });
      },
      &media_destinations);
  MediaActionExecutor media_executor(media_backend);

  auto render_download_panel = [&] {
    if (!live_downloads) {
      host.show_panel("Advanced Download Manager\nLive transfer runtime is unavailable in this build.");
      return;
    }
    const auto model = AdvancedDownloadPanelBuilder::build(*live_downloads);
    host.show_panel(AdvancedDownloadPanelBuilder::format_text(model));
  };

  auto present_active_tab = [&] {
    auto* tab = window->active_tab();
    if (!tab) return;
    const auto navigation = tab->engine_view().navigation_state();
    if (is_goreecloud_internal_url(navigation.url)) {
      host.show_internal_surface(navigation.url);
    } else {
      host.attach_engine_view(tab->engine_view());
    }
  };

  host.set_tab_action_handler([&](GtkTabAction action, std::string_view tab_id) {
    commands.clear_panel();
    switch (action) {
      case GtkTabAction::activate:
        (void)window->activate_tab(tab_id);
        break;
      case GtkTabAction::close:
        if (window->close_tab(tab_id, true) && window->tab_count() == 0) {
          (void)window->new_tab();
        }
        break;
      case GtkTabAction::create:
        (void)window->new_tab();
        break;
      case GtkTabAction::next:
        (void)window->activate_next_tab();
        break;
      case GtkTabAction::previous:
        (void)window->activate_previous_tab();
        break;
    }
    present_active_tab();
  });

  host.set_toolbar_handler([&](ToolbarItem item) {
    commands.clear_panel();
    if (!commands.invoke(item)) return;
    if (commands.active_panel() == "downloads") {
      render_download_panel();
    } else if (!commands.active_panel().empty()) {
      host.show_panel(commands.active_panel());
    }
  });

  host.set_search_handler([&](std::string_view input) {
    try {
      const auto resolution = omnibox.resolve(input);
      if (resolution.intent == OmniboxIntent::empty) return;
      commands.clear_panel();
      window->navigate_active(resolution.value);
      if (is_goreecloud_internal_url(resolution.value)) {
        host.show_internal_surface(resolution.value);
      } else if (auto* tab = window->active_tab()) {
        host.attach_engine_view(tab->engine_view());
      }
    } catch (const std::exception&) {
      host.show_panel("search-unavailable");
    }
  });

  host.set_search_control_handler([&](UnifiedSearchBarControl control) {
    switch (control) {
      case UnifiedSearchBarControl::advanced_reader_mode:
        host.show_panel("reader-mode");
        break;
      case UnifiedSearchBarControl::unified_bookmarks:
        host.show_panel("bookmarks");
        break;
      case UnifiedSearchBarControl::wardveil_security:
        host.show_panel("wardveil-security");
        break;
    }
  });

  host.set_media_hover_action_handler([&](MediaAction action, const MediaTarget& target) {
    MediaActionRequest request;
    request.action = action;
    request.target = target;
    request.explicit_user_action = true;

    if (action == MediaAction::search || action == MediaAction::search_similar ||
        action == MediaAction::search_region || action == MediaAction::search_frame) {
      request.processing_destination = MediaProcessingDestination::goreecloud_hosted;
      request.privacy_authorized = host.confirm_media_boundary(
          "Privacy Shield — Visual Search",
          "This action will send the selected media reference to GoreeCloud Search for visual processing. Continue?");
      if (!request.privacy_authorized) return;
    }

    if (action == MediaAction::save || action == MediaAction::download_media ||
        action == MediaAction::save_region || action == MediaAction::save_frame) {
      const auto destination = host.choose_media_save_destination(target.kind);
      if (!destination) return;
      request.save_destination = *destination;
      if (*destination != MediaSaveDestination::local_device) {
        request.persistence_warning_accepted = host.confirm_media_boundary(
            "Private-to-Persistent Boundary",
            "This synchronized destination will preserve the media outside the current webpage/session when its GoreeCloud service adapter is available. Continue?");
        if (!request.persistence_warning_accepted) return;
      }
    }

    if (action == MediaAction::save_to_drive || action == MediaAction::save_to_photos ||
        action == MediaAction::save_to_video || action == MediaAction::save_to_notes ||
        action == MediaAction::save_to_memos || action == MediaAction::create_task) {
      request.persistence_warning_accepted = host.confirm_media_boundary(
          "Private-to-Persistent Boundary",
          "This action will intentionally preserve the media reference in another GoreeCloud service. Continue?");
      if (!request.persistence_warning_accepted) return;
    }

    const auto result = media_executor.execute(request, media_policy);
    if (!result.message.empty() && action != MediaAction::preview) {
      host.show_media_action_status(result.message);
    }
  });

  if (!host.create()) {
    throw std::runtime_error(
        "Linux beta host requires GTK3 on an X11/XWayland display; native Wayland embedding is not yet enabled");
  }

  host.set_title(window->private_window() ? "GoreeCloud Browser — Private" : "GoreeCloud Browser");
  host.render_chrome(chrome.snapshot());

  host.show();
  present_active_tab();

  auto* initial_tab = window->active_tab();
  std::string last_tab_id = initial_tab ? initial_tab->id() : std::string{};
  std::string last_url =
      initial_tab ? initial_tab->engine_view().navigation_state().url : std::string{};

  while (host.pump_events()) {
    application.engine().pump_events();
    download_runtime->pump();
    if (commands.active_panel() == "downloads") render_download_panel();

    host.render_chrome(chrome.snapshot());
    if (auto* tab = window->active_tab()) {
      const auto navigation = tab->engine_view().navigation_state();
      if (tab->id() != last_tab_id || navigation.url != last_url) {
        last_tab_id = tab->id();
        last_url = navigation.url;
        present_active_tab();
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  host.close();
  return 0;
}

}  // namespace goreecloud::browser::platform
