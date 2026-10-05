#include <algorithm>
#include <iostream>
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

#define GC_REQUIRE(condition)                                                   \
  do {                                                                          \
    if (!(condition)) {                                                         \
      std::cerr << "runtime smoke requirement failed: " #condition             \
                << " at " << __FILE__ << ":" << __LINE__ << std::endl;         \
      return 1;                                                                 \
    }                                                                           \
  } while (false)

int main() {
  using namespace goreecloud::browser;

  static_assert(kDefaultToolbar.size() == 12);
  static_assert(kUnifiedSearchBarControls.size() == 3);
  static_assert(kCurrentGlazeUiStableVersion == std::string_view{"1.7.0"});
  static_assert(kCurrentGlazeUiStableRevision ==
                std::string_view{"1a5756daed2294155be2e9972b24f580f6222b7b"});
  static_assert(kCurrentGlazeUiQualificationSourceAnchor ==
                std::string_view{"7c4ded83d7a8725165bb6a55dfb175667cc9589e"});
  static_assert(kCurrentGlazeUiQualificationIntegrationRevision ==
                std::string_view{"1a5756daed2294155be2e9972b24f580f6222b7b"});
  static_assert(kGlazeUiImmediateRollbackVersion == std::string_view{"1.6.0"});
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
    GC_REQUIRE(!service_capability_available(search_health, "search.query", "1"));

    search_health.capabilities.front().production_accepted = true;
    GC_REQUIRE(service_capability_available(search_health, "search.query", "1"));
    GC_REQUIRE(!service_capability_available(search_health, "search.query", "2"));
    GC_REQUIRE(!service_capability_available(search_health, "vault.secrets", "1"));

    search_health.capabilities.front().contract_version.clear();
    GC_REQUIRE(!service_capability_available(search_health, "search.query"));
    search_health.capabilities.front().contract_version = "1";

    search_health.capabilities.push_back(CapabilityEvidence{
        .id = "search.query",
        .contract_version = "1",
        .authoritative = false,
        .current = true,
        .production_accepted = false,
    });
    GC_REQUIRE(!service_capability_available(search_health, "search.query", "1"));
    search_health.capabilities.pop_back();

    search_health.capabilities.front().current = false;
    GC_REQUIRE(!service_capability_available(search_health, "search.query", "1"));
    search_health.capabilities.front().current = true;
    search_health.capabilities.front().authoritative = false;
    GC_REQUIRE(!service_capability_available(search_health, "search.query", "1"));
    search_health.capabilities.front().authoritative = true;
    search_health.status = ServiceStatus::degraded;
    GC_REQUIRE(!service_capability_available(search_health, "search.query", "1"));
  }

  {
    char executable[] = "goreecloud-browser";
    char private_flag[] = "--private";
    char url[] = "https://example.com/";
    char* argv[]{executable, private_flag, url};
    const auto launch = parse_browser_launch_request(3, argv);
    GC_REQUIRE(launch.private_window);
    GC_REQUIRE(!launch.isolated_private_window);
    GC_REQUIRE(launch.urls.size() == 1);
    GC_REQUIRE(launch.urls.front() == "https://example.com/");
  }

  ConfiguredGoreeCloudSearchRouter search_router("https://search.goreecloud.test/search");
  OmniboxController omnibox(search_router);
  const auto url_resolution = omnibox.resolve("example.com");
  GC_REQUIRE(url_resolution.intent == OmniboxIntent::direct_navigation);
  GC_REQUIRE(url_resolution.value == "https://example.com");
  const auto search_resolution = omnibox.resolve("goreecloud browser beta");
  GC_REQUIRE(search_resolution.intent == OmniboxIntent::goreecloud_search);
  GC_REQUIRE(search_resolution.value.find("https://search.goreecloud.test/search?q=") == 0);

  GC_REQUIRE(browser_tab_title(kNewTabUrl, kNewTabUrl) == "New Tab");
  GC_REQUIRE(browser_tab_title(kHomeUrl, kHomeUrl) == "Home");
  GC_REQUIRE(browser_location_text(kNewTabUrl).empty());
  GC_REQUIRE(browser_location_text(kHomeUrl).empty());
  GC_REQUIRE(browser_location_text("https://example.com/") == "https://example.com/");

  {
    const auto bookmarks_panel = browser_panel_presentation("bookmarks");
    GC_REQUIRE(bookmarks_panel.title == "Bookmarks");
    GC_REQUIRE(bookmarks_panel.eyebrow == "LIBRARY");
    GC_REQUIRE(bookmarks_panel.body.find("Local Bookmarks") != std::string::npos);

    const auto downloads_panel = browser_panel_presentation(
        "Advanced Download Manager\nActive 0  Completed 0\nNo downloads.");
    GC_REQUIRE(downloads_panel.title == "Downloads");
    GC_REQUIRE(downloads_panel.body.find("No downloads.") != std::string::npos);

    const auto security_panel = browser_panel_presentation("wardveil-security");
    GC_REQUIRE(security_panel.title == "Security");
    GC_REQUIRE(security_panel.status.find("pending") != std::string::npos);
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
    GC_REQUIRE(normalized);
    GC_REQUIRE(normalized->kind == MediaKind::image);
    GC_REQUIRE(normalized->media_url == hit.media_url);
    GC_REQUIRE(normalized->link_url == hit.link_url);
    GC_REQUIRE(normalized->can_download);
    GC_REQUIRE(normalized->can_copy);

    hit.drm_protected = true;
    hit.capturable_frame = true;
    const auto protected_target = MediaTargetDetector::normalize(hit);
    GC_REQUIRE(protected_target);
    GC_REQUIRE(protected_target->protected_media);
    GC_REQUIRE(!protected_target->can_download);
    GC_REQUIRE(!protected_target->can_capture_frame);
  }

  {
    MediaProbeResultTracker tracker;
    const auto first_sequence = tracker.next_sequence();
    const auto second_sequence = tracker.next_sequence();
    GC_REQUIRE(second_sequence > first_sequence);

    EngineMediaHitTest stale;
    stale.kind = EngineMediaElementKind::image;
    stale.media_url = "https://example.com/stale.jpg";
    const bool stale_accepted = tracker.accept(first_sequence, stale);
    GC_REQUIRE(!stale_accepted);
    GC_REQUIRE(!tracker.latest_result());

    EngineMediaHitTest current;
    current.kind = EngineMediaElementKind::image;
    current.media_url = "https://example.com/current.jpg";
    const bool current_accepted = tracker.accept(second_sequence, current);
    GC_REQUIRE(current_accepted);
    GC_REQUIRE(tracker.latest_result());
    GC_REQUIRE(tracker.latest_result()->media_url == current.media_url);
    GC_REQUIRE(tracker.latest_accepted() == second_sequence);
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
    GC_REQUIRE(std::find(actions.begin(), actions.end(), MediaAction::preview) != actions.end());
    GC_REQUIRE(std::find(actions.begin(), actions.end(), MediaAction::search) != actions.end());
    GC_REQUIRE(std::find(actions.begin(), actions.end(), MediaAction::open_link) != actions.end());
    GC_REQUIRE(std::find(actions.begin(), actions.end(), MediaAction::copy_media_url) != actions.end());

    const auto denied_remote = MediaProcessingPolicy::decide(
        MediaAction::search, MediaProcessingDestination::goreecloud_hosted, policy);
    GC_REQUIRE(!denied_remote.allowed);

    policy.allow_remote_processing = true;
    const auto allowed_remote = MediaProcessingPolicy::decide(
        MediaAction::search, MediaProcessingDestination::goreecloud_hosted, policy);
    GC_REQUIRE(allowed_remote.allowed);
    GC_REQUIRE(allowed_remote.disclosure_required);

    const auto view_model = MediaHoverViewModelBuilder::build(
        image,
        policy,
        false,
        true,
        true,
        MediaSaveDestination::goreecloud_drive,
        "Privacy Shield: Remote processing allowed",
        "Wardveil: Secure resource");
    GC_REQUIRE(view_model.visible);
    GC_REQUIRE(view_model.reduced_motion);
    GC_REQUIRE(view_model.keyboard_focus_visible);
    GC_REQUIRE(view_model.quick_actions[0].label == "Preview");
    GC_REQUIRE(view_model.quick_actions[1].label == "Search");
    GC_REQUIRE(view_model.quick_actions[2].label == "Save");
    GC_REQUIRE(view_model.quick_actions[3].label == "More");
    GC_REQUIRE(view_model.destination.visible);
    GC_REQUIRE(view_model.destination.label.find("Synchronized") != std::string::npos);
    GC_REQUIRE(view_model.privacy.visible);
    GC_REQUIRE(view_model.security.visible);

    image.protected_media = true;
    const auto protected_actions = MediaActionRegistry::actions_for(image, policy);
    GC_REQUIRE(std::find(protected_actions.begin(), protected_actions.end(),
                     MediaAction::download_media) == protected_actions.end());

    GC_REQUIRE(save_destination_label(MediaSaveDestination::local_device).find("Local") !=
           std::string_view::npos);
    GC_REQUIRE(save_destination_label(MediaSaveDestination::goreecloud_drive).find("Synchronized") !=
           std::string_view::npos);

    MediaHoverController hover;
    const bool keyboard_activated = hover.activate(
        image, MediaHoverActivation::keyboard_focus, policy, false);
    GC_REQUIRE(keyboard_activated);
    GC_REQUIRE(hover.visible());
    const auto placement = MediaHoverController::place(
        MediaRect{.x = 980, .y = 20, .width = 300, .height = 200},
        MediaViewport{.width = 1024, .height = 768},
        220,
        48);
    GC_REQUIRE(placement.visible);
    GC_REQUIRE(placement.x >= 8);
    GC_REQUIRE(placement.x + 220 <= 1024 - 8);
    hover.pointer_left_media(false);
    GC_REQUIRE(!hover.visible());

    policy.modifier_required = true;
    const bool unmodified = hover.activate(
        image, MediaHoverActivation::pointer_hover, policy, false);
    const bool modified = hover.activate(
        image, MediaHoverActivation::pointer_hover, policy, true);
    GC_REQUIRE(!unmodified);
    GC_REQUIRE(modified);
  }

  DevelopmentEngine engine;
  engine.initialize();

  EngineContextOptions normal_options;
  normal_options.profile_id = "smoke-normal";
  normal_options.storage_path = "smoke-profile";
  normal_options.private_context = false;
  normal_options.persistent_storage = true;

  auto normal_context = engine.create_context(normal_options);
  GC_REQUIRE(normal_context);

  InMemoryAdvancedTabManager tab_manager;
  WindowController window(*normal_context, false, &tab_manager, "window-smoke");

  auto& first = window.new_tab(std::string{kNewTabUrl});
  tab_manager.register_tab(ManagedTabState{
      .tab_id = first.id(),
      .window_id = window.window_id(),
      .workspace_id = "workspace-main",
      .group_id = std::nullopt,
      .split_id = std::nullopt,
      .pinned = false,
      .protection = TabProtection::normal,
      .sleep_policy = TabSleepPolicy::automatic,
      .resources = {},
  });
  auto& second = window.new_tab("https://example.com/");
  tab_manager.register_tab(ManagedTabState{
      .tab_id = second.id(),
      .window_id = window.window_id(),
      .workspace_id = "workspace-main",
      .group_id = std::nullopt,
      .split_id = std::nullopt,
      .pinned = false,
      .protection = TabProtection::normal,
      .sleep_policy = TabSleepPolicy::automatic,
      .resources = {},
  });

  if (window.tab_count() != 2 || window.tab_views().size() != 2) return 1;
  BrowserChromeShell smoke_chrome(window);
  const auto initial_chrome = smoke_chrome.snapshot();
  if (initial_chrome.tabs.size() != 2 || !initial_chrome.tabs.back().active) {
    return 1;
  }

  if (!window.move_active_tab_left()) return 1;
  const auto reordered_left = window.tab_ids();
  if (reordered_left.size() != 2 ||
      reordered_left[0] != second.id() ||
      reordered_left[1] != first.id() ||
      !window.active_tab() ||
      window.active_tab()->id() != second.id()) {
    return 1;
  }

  if (!window.move_active_tab_right()) return 1;
  const auto reordered_right = window.tab_ids();
  if (reordered_right.size() != 2 ||
      reordered_right[0] != first.id() ||
      reordered_right[1] != second.id() ||
      !window.active_tab() ||
      window.active_tab()->id() != second.id()) {
    return 1;
  }

  GC_REQUIRE(window.activate_previous_tab());
  GC_REQUIRE(window.active_tab() && window.active_tab()->id() == first.id());
  GC_REQUIRE(window.activate_next_tab());
  GC_REQUIRE(window.active_tab() && window.active_tab()->id() == second.id());
  const bool selected_first = window.select_tab(first.id(), false);
  const bool selected_second = window.select_tab(second.id(), true);
  const bool pinned = window.pin_selected_tabs(true);
  const bool protected_tabs = window.protect_selected_tabs(true);
  const bool regular_close = window.close_tab(second.id());
  const bool explicit_close = window.close_tab(second.id(), true);
  GC_REQUIRE(selected_first);
  GC_REQUIRE(selected_second);
  GC_REQUIRE(pinned);
  GC_REQUIRE(protected_tabs);
  GC_REQUIRE(!regular_close);
  GC_REQUIRE(explicit_close);
  GC_REQUIRE(window.tab_count() == 1);

  window.open_home();
  GC_REQUIRE(window.active_tab());
  GC_REQUIRE(window.active_tab()->engine_view().navigation_state().url == kHomeUrl);
  window.open_settings();
  GC_REQUIRE(window.active_tab()->engine_view().navigation_state().url == kSettingsUrl);

  engine.shutdown();

  BrowserApplicationOptions private_options;
  private_options.initial_private_window = true;
  private_options.initial_private_session_id = "smoke-private";
  private_options.initial_url = std::string{kPrivateStartUrl};
  BrowserApplication private_browser(std::make_unique<DevelopmentEngine>(), private_options);
  private_browser.initialize();
  GC_REQUIRE(private_browser.window_count() == 1);
  GC_REQUIRE(private_browser.first_window());
  GC_REQUIRE(private_browser.first_window()->private_window());
  GC_REQUIRE(private_browser.first_window()->active_tab());
  GC_REQUIRE(private_browser.first_window()->active_tab()->engine_view().navigation_state().url ==
         kPrivateStartUrl);
  GC_REQUIRE(private_browser.has_private_session_context("smoke-private"));
  GC_REQUIRE(!private_browser.has_default_context());
  GC_REQUIRE(private_browser.first_window()->private_session_id() == "smoke-private");

  auto& same_private_session =
      private_browser.new_private_window("smoke-private");
  (void)same_private_session.new_tab(std::string{kPrivateStartUrl});
  GC_REQUIRE(same_private_session.private_session_id() == "smoke-private");

  auto& isolated_private_session =
      private_browser.new_private_window("isolated-smoke");
  (void)isolated_private_session.new_tab(std::string{kPrivateStartUrl});
  GC_REQUIRE(isolated_private_session.private_session_id() == "isolated-smoke");
  GC_REQUIRE(private_browser.has_private_session_context("isolated-smoke"));

  auto& normal_after_private = private_browser.new_window(false);
  GC_REQUIRE(!normal_after_private.private_window());
  GC_REQUIRE(normal_after_private.private_session_id().empty());
  GC_REQUIRE(private_browser.has_default_context());
  GC_REQUIRE(private_browser.window_count() == 4);

  GC_REQUIRE(private_browser.close_private_session("smoke-private"));
  GC_REQUIRE(!private_browser.has_private_session_context("smoke-private"));
  GC_REQUIRE(private_browser.has_private_session_context("isolated-smoke"));
  GC_REQUIRE(private_browser.has_default_context());
  GC_REQUIRE(private_browser.window_count() == 2);
  GC_REQUIRE(private_browser.first_window());
  GC_REQUIRE(private_browser.first_window()->private_window());
  GC_REQUIRE(private_browser.first_window()->private_session_id() ==
             "isolated-smoke");
  GC_REQUIRE(!private_browser.close_private_session("missing-private"));

  GC_REQUIRE(private_browser.close_private_session("isolated-smoke"));
  GC_REQUIRE(!private_browser.has_private_session_context("isolated-smoke"));
  GC_REQUIRE(private_browser.has_default_context());
  GC_REQUIRE(private_browser.window_count() == 1);
  GC_REQUIRE(private_browser.first_window());
  GC_REQUIRE(!private_browser.first_window()->private_window());
  GC_REQUIRE(private_browser.first_window()->private_session_id().empty());

  private_browser.shutdown();

  return 0;
}
