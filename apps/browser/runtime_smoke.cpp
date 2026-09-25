#include <algorithm>
#include <cassert>
#include <memory>
#include <string>

#include "goreecloud/browser/application.hpp"
#include "goreecloud/browser/command_line.hpp"
#include "goreecloud/browser/chrome_shell.hpp"
#include "goreecloud/browser/configured_search_router.hpp"
#include "goreecloud/browser/development_engine.hpp"
#include "goreecloud/browser/in_memory_tab_manager.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/glaze.hpp"
#include "goreecloud/browser/media_hover.hpp"
#include "goreecloud/browser/media_hover_controller.hpp"
#include "goreecloud/browser/media_hover_ui.hpp"
#include "goreecloud/browser/media_probe_result_tracker.hpp"
#include "goreecloud/browser/media_target_detector.hpp"
#include "goreecloud/browser/omnibox_controller.hpp"
#include "goreecloud/browser/panel_surface.hpp"
#include "goreecloud/browser/services.hpp"
#include "goreecloud/browser/toolbar.hpp"
#include "goreecloud/browser/unified_search_bar.hpp"
#include "goreecloud/browser/version.hpp"
#include "goreecloud/browser/window_controller.hpp"

int main() {
  using namespace goreecloud::browser;

  static_assert(kDefaultToolbar.size() == 12);
  static_assert(kUnifiedSearchBarControls.size() == 3);
  static_assert(kCurrentGlazeUiStableVersion == std::string_view{"1.6.0"});
  static_assert(kCurrentGlazeUiStableRevision ==
                std::string_view{"a7180679ea851389e0f3004515f9a25f420e716d"});
  static_assert(kCurrentGlazeUiQualificationSourceAnchor ==
                std::string_view{"c7509c79256b04b0aa67cb9dd0737d7588e0ae4a"});
  static_assert(kCurrentGlazeUiQualificationIntegrationRevision ==
                std::string_view{"354f5759385c28596fcfec26a3ad525e89fb1c35"});
  static_assert(kGlazeUiImmediateRollbackVersion == std::string_view{"1.5.1"});
  static_assert(kBrowserGlazeCapabilities.complete_component_states);
  static_assert(kBrowserGlazeCapabilities.large_text_reflow);
  static_assert(kBrowserGlazeCapabilities.non_color_semantic_indicators);
  static_assert(kBrowserGlazeCapabilities.responsive_task_continuity);
  static_assert(!kAlternateSearchProvidersAllowed);
  static_assert(!kSilentSearchFallbackAllowed);
  static_assert(kBetaChannel);
  static_assert(!kProductionApproved);
  static_assert(!kMediaHoverPassiveUploadAllowed);
  static_assert(!kMediaHoverPassiveAiAnalysisAllowed);
  static_assert(!kMediaHoverPassiveOcrAllowed);
  static_assert(!kMediaDetectorMayOverrideProtectedMedia);
  static_assert(kDefaultMediaHoverQuickLabels.size() == 4);

  {
    ServiceHealth search_health;
    search_health.status = ServiceStatus::available;
    search_health.capabilities.push_back(CapabilityEvidence{
        .id = "search.query",
        .contract_version = "1",
        .authoritative = true,
        .current = true,
        .production_accepted = false,
    });
    assert(!service_capability_available(search_health, "search.query", "1"));

    search_health.capabilities.front().production_accepted = true;
    assert(service_capability_available(search_health, "search.query", "1"));
    assert(!service_capability_available(search_health, "search.query", "2"));
    assert(!service_capability_available(search_health, "vault.secrets", "1"));

    search_health.capabilities.front().contract_version.clear();
    assert(!service_capability_available(search_health, "search.query"));
    search_health.capabilities.front().contract_version = "1";

    search_health.capabilities.push_back(CapabilityEvidence{
        .id = "search.query",
        .contract_version = "1",
        .authoritative = false,
        .current = true,
        .production_accepted = false,
    });
    assert(!service_capability_available(search_health, "search.query", "1"));
    search_health.capabilities.pop_back();

    search_health.capabilities.front().current = false;
    assert(!service_capability_available(search_health, "search.query", "1"));
    search_health.capabilities.front().current = true;
    search_health.capabilities.front().authoritative = false;
    assert(!service_capability_available(search_health, "search.query", "1"));
    search_health.capabilities.front().authoritative = true;
    search_health.status = ServiceStatus::degraded;
    assert(!service_capability_available(search_health, "search.query", "1"));
  }

  {
    char executable[] = "goreecloud-browser";
    char private_flag[] = "--private";
    char url[] = "https://example.com/";
    char* argv[]{executable, private_flag, url};
    const auto launch = parse_browser_launch_request(3, argv);
    assert(launch.private_window);
    assert(!launch.isolated_private_window);
    assert(launch.urls.size() == 1);
    assert(launch.urls.front() == "https://example.com/");
  }

  ConfiguredGoreeCloudSearchRouter search_router("https://search.goreecloud.test/search");
  OmniboxController omnibox(search_router);
  const auto url_resolution = omnibox.resolve("example.com");
  assert(url_resolution.intent == OmniboxIntent::direct_navigation);
  assert(url_resolution.value == "https://example.com");
  const auto search_resolution = omnibox.resolve("goreecloud browser beta");
  assert(search_resolution.intent == OmniboxIntent::goreecloud_search);
  assert(search_resolution.value.find("https://search.goreecloud.test/search?q=") == 0);

  assert(browser_tab_title(kNewTabUrl, kNewTabUrl) == "New Tab");
  assert(browser_tab_title(kHomeUrl, kHomeUrl) == "Home");
  assert(browser_location_text(kNewTabUrl).empty());
  assert(browser_location_text(kHomeUrl).empty());
  assert(browser_location_text("https://example.com/") == "https://example.com/");

  {
    const auto bookmarks_panel = browser_panel_presentation("bookmarks");
    assert(bookmarks_panel.title == "Bookmarks");
    assert(bookmarks_panel.eyebrow == "LIBRARY");
    assert(bookmarks_panel.body.find("Local Bookmarks") != std::string::npos);

    const auto downloads_panel = browser_panel_presentation(
        "Advanced Download Manager\nActive 0  Completed 0\nNo downloads.");
    assert(downloads_panel.title == "Downloads");
    assert(downloads_panel.body.find("No downloads.") != std::string::npos);

    const auto security_panel = browser_panel_presentation("wardveil-security");
    assert(security_panel.title == "Security");
    assert(security_panel.status.find("pending") != std::string::npos);
  }

  {
    EngineMediaHitTest hit;
    hit.kind = EngineMediaElementKind::image;
    hit.page_url = "https://example.com/gallery";
    hit.page_title = "Gallery";
    hit.media_url = "https://cdn.example.com/image.jpg";
    hit.link_url = "https://example.com/destination";
    hit.mime_type = "image/jpeg";
    hit.alt_text = "Example image";
    hit.intrinsic_width = 1920;
    hit.intrinsic_height = 1080;
    hit.displayed_width = 640;
    hit.displayed_height = 360;
    hit.secure_resource = true;
    hit.downloadable = true;
    hit.copyable = true;

    const auto normalized = MediaTargetDetector::normalize(hit);
    assert(normalized);
    assert(normalized->kind == MediaKind::image);
    assert(normalized->media_url == hit.media_url);
    assert(normalized->link_url == hit.link_url);
    assert(normalized->can_download);
    assert(normalized->can_copy);

    hit.drm_protected = true;
    hit.capturable_frame = true;
    const auto protected_target = MediaTargetDetector::normalize(hit);
    assert(protected_target);
    assert(protected_target->protected_media);
    assert(!protected_target->can_download);
    assert(!protected_target->can_capture_frame);
  }

  {
    MediaProbeResultTracker tracker;
    const auto first_sequence = tracker.next_sequence();
    const auto second_sequence = tracker.next_sequence();
    assert(second_sequence > first_sequence);

    EngineMediaHitTest stale;
    stale.kind = EngineMediaElementKind::image;
    stale.media_url = "https://example.com/stale.jpg";
    const bool stale_accepted = tracker.accept(first_sequence, stale);
    assert(!stale_accepted);
    assert(!tracker.latest_result());

    EngineMediaHitTest current;
    current.kind = EngineMediaElementKind::image;
    current.media_url = "https://example.com/current.jpg";
    const bool current_accepted = tracker.accept(second_sequence, current);
    assert(current_accepted);
    assert(tracker.latest_result());
    assert(tracker.latest_result()->media_url == current.media_url);
    assert(tracker.latest_accepted() == second_sequence);
  }

  {
    MediaTarget image;
    image.page_url = "https://example.com/gallery";
    image.media_url = "https://cdn.example.com/image.jpg";
    image.link_url = "https://example.com/destination";
    image.kind = MediaKind::image;
    image.secure_resource = true;

    MediaHoverSitePolicy policy;
    policy.allow_remote_processing = false;

    const auto actions = MediaActionRegistry::actions_for(image, policy);
    assert(std::find(actions.begin(), actions.end(), MediaAction::preview) != actions.end());
    assert(std::find(actions.begin(), actions.end(), MediaAction::search) != actions.end());
    assert(std::find(actions.begin(), actions.end(), MediaAction::open_link) != actions.end());
    assert(std::find(actions.begin(), actions.end(), MediaAction::copy_media_url) != actions.end());

    const auto denied_remote = MediaProcessingPolicy::decide(
        MediaAction::search, MediaProcessingDestination::goreecloud_hosted, policy);
    assert(!denied_remote.allowed);

    policy.allow_remote_processing = true;
    const auto allowed_remote = MediaProcessingPolicy::decide(
        MediaAction::search, MediaProcessingDestination::goreecloud_hosted, policy);
    assert(allowed_remote.allowed);
    assert(allowed_remote.disclosure_required);

    const auto view_model = MediaHoverViewModelBuilder::build(
        image,
        policy,
        false,
        true,
        true,
        MediaSaveDestination::goreecloud_drive,
        "Privacy Shield: Remote processing allowed",
        "Wardveil: Secure resource");
    assert(view_model.visible);
    assert(view_model.reduced_motion);
    assert(view_model.keyboard_focus_visible);
    assert(view_model.quick_actions[0].label == "Preview");
    assert(view_model.quick_actions[1].label == "Search");
    assert(view_model.quick_actions[2].label == "Save");
    assert(view_model.quick_actions[3].label == "More");
    assert(view_model.destination.visible);
    assert(view_model.destination.label.find("Synchronized") != std::string::npos);
    assert(view_model.privacy.visible);
    assert(view_model.security.visible);

    image.protected_media = true;
    const auto protected_actions = MediaActionRegistry::actions_for(image, policy);
    assert(std::find(protected_actions.begin(), protected_actions.end(),
                     MediaAction::download_media) == protected_actions.end());

    assert(save_destination_label(MediaSaveDestination::local_device).find("Local") !=
           std::string_view::npos);
    assert(save_destination_label(MediaSaveDestination::goreecloud_drive).find("Synchronized") !=
           std::string_view::npos);

    MediaHoverController hover;
    const bool keyboard_activated = hover.activate(
        image, MediaHoverActivation::keyboard_focus, policy, false);
    assert(keyboard_activated);
    assert(hover.visible());
    const auto placement = MediaHoverController::place(
        MediaRect{.x = 980, .y = 20, .width = 300, .height = 200},
        MediaViewport{.width = 1024, .height = 768},
        220,
        48);
    assert(placement.visible);
    assert(placement.x >= 8);
    assert(placement.x + 220 <= 1024 - 8);
    hover.pointer_left_media(false);
    assert(!hover.visible());

    policy.modifier_required = true;
    const bool unmodified = hover.activate(
        image, MediaHoverActivation::pointer_hover, policy, false);
    const bool modified = hover.activate(
        image, MediaHoverActivation::pointer_hover, policy, true);
    assert(!unmodified);
    assert(modified);
  }

  DevelopmentEngine engine;
  engine.initialize();

  EngineContextOptions normal_options;
  normal_options.profile_id = "smoke-normal";
  normal_options.storage_path = "smoke-profile";
  normal_options.private_context = false;
  normal_options.persistent_storage = true;

  auto normal_context = engine.create_context(normal_options);
  assert(normal_context);

  InMemoryAdvancedTabManager tab_manager;
  WindowController window(*normal_context, false, &tab_manager, "window-smoke");

  auto& first = window.new_tab(std::string{kNewTabUrl});
  tab_manager.register_tab({first.id(), window.window_id(), "workspace-main"});
  auto& second = window.new_tab("https://example.com/");
  tab_manager.register_tab({second.id(), window.window_id(), "workspace-main"});

  assert(window.tab_count() == 2);
  assert(window.tab_views().size() == 2);
  BrowserChromeShell smoke_chrome(window);
  const auto initial_chrome = smoke_chrome.snapshot();
  assert(initial_chrome.tabs.size() == 2);
  assert(initial_chrome.tabs.back().active);
  assert(window.activate_previous_tab());
  assert(window.active_tab() && window.active_tab()->id() == first.id());
  assert(window.activate_next_tab());
  assert(window.active_tab() && window.active_tab()->id() == second.id());
  const bool selected_first = window.select_tab(first.id(), false);
  const bool selected_second = window.select_tab(second.id(), true);
  const bool pinned = window.pin_selected_tabs(true);
  const bool protected_tabs = window.protect_selected_tabs(true);
  const bool regular_close = window.close_tab(second.id());
  const bool explicit_close = window.close_tab(second.id(), true);
  assert(selected_first);
  assert(selected_second);
  assert(pinned);
  assert(protected_tabs);
  assert(!regular_close);
  assert(explicit_close);
  assert(window.tab_count() == 1);

  window.open_home();
  assert(window.active_tab());
  assert(window.active_tab()->engine_view().navigation_state().url == kHomeUrl);
  window.open_settings();
  assert(window.active_tab()->engine_view().navigation_state().url == kSettingsUrl);

  engine.shutdown();

  BrowserApplicationOptions private_options;
  private_options.initial_private_window = true;
  private_options.initial_private_session_id = "smoke-private";
  private_options.initial_url = std::string{kPrivateStartUrl};
  BrowserApplication private_browser(std::make_unique<DevelopmentEngine>(), private_options);
  private_browser.initialize();
  assert(private_browser.window_count() == 1);
  assert(private_browser.first_window());
  assert(private_browser.first_window()->private_window());
  assert(private_browser.first_window()->active_tab());
  assert(private_browser.first_window()->active_tab()->engine_view().navigation_state().url ==
         kPrivateStartUrl);
  assert(private_browser.has_private_session_context("smoke-private"));
  private_browser.shutdown();

  return 0;
}
