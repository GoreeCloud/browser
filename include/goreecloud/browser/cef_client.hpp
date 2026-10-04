#pragma once

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "goreecloud/browser/cef_media_probe.hpp"
#include "goreecloud/browser/engine.hpp"
#include "goreecloud/browser/media_preview_provider.hpp"
#include "goreecloud/browser/media_target_detector.hpp"
#include "goreecloud/browser/native_engine_surface.hpp"

#if GOREECLOUD_ENABLE_CEF
#include "include/cef_client.h"
#include "include/cef_context_menu_handler.h"
#include "include/cef_drag_data.h"
#include "include/cef_process_message.h"
#include "include/cef_render_handler.h"
#include "include/wrapper/cef_helpers.h"
#endif

namespace goreecloud::browser {

#if GOREECLOUD_ENABLE_CEF

class GoreeCloudCefClient final : public CefClient,
                                  public CefDisplayHandler,
                                  public CefLifeSpanHandler,
                                  public CefLoadHandler,
                                  public CefContextMenuHandler,
                                  public CefRenderHandler {
 public:
  using NavigationCallback = std::function<void(const NavigationState&)>;
  using ClosedCallback = std::function<void()>;
  using MediaContextCallback = std::function<void(const RawMediaHitTest&)>;
  using MediaProbeCallback =
      std::function<void(std::uint64_t, std::optional<RawMediaHitTest>)>;
  using MediaPreviewCallback = AsyncMediaPreviewProvider::PreviewCallback;

  GoreeCloudCefClient(NavigationCallback navigation_callback,
                      ClosedCallback closed_callback,
                      MediaContextCallback media_context_callback = {})
      : navigation_callback_(std::move(navigation_callback)),
        closed_callback_(std::move(closed_callback)),
        media_context_callback_(std::move(media_context_callback)) {}

  CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
  CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }
  CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

  void configure_windowless_surface(
      NativeSurfaceFrameSink* sink,
      NativeSurfaceCursorSink* cursor_sink,
      NativeSurfaceContextMenuSink* context_menu_sink,
      int width,
      int height,
      float scale_factor) {
    std::scoped_lock lock(render_mutex_);
    frame_sink_ = sink;
    cursor_sink_ = cursor_sink;
    context_menu_sink_ = context_menu_sink;
    view_width_ = std::max(1, width);
    view_height_ = std::max(1, height);
    scale_factor_ = std::max(0.25F, scale_factor);
  }

  void clear_windowless_surface() {
    NativeSurfaceFrameSink* sink = nullptr;
    {
      std::scoped_lock lock(render_mutex_);
      sink = frame_sink_;
      frame_sink_ = nullptr;
      cursor_sink_ = nullptr;
      context_menu_sink_ = nullptr;
      popup_visible_ = false;
      popup_rect_ = CefRect();
    }
    if (sink) sink->clear_software_popup_frame();
  }

  bool begin_windowless_drop(
      CefRefPtr<CefDragData> drag_data,
      const CefMouseEvent& event,
      CefBrowserHost::DragOperationsMask allowed_ops) {
    CEF_REQUIRE_UI_THREAD();
    if (!browser_ || !drag_data || allowed_ops == DRAG_OPERATION_NONE) {
      return false;
    }

    auto host = browser_->GetHost();
    if (!host) return false;

    if (pending_drop_active_) {
      host->DragTargetDragLeave();
      pending_drop_active_ = false;
      pending_drop_allowed_ops_ = DRAG_OPERATION_NONE;
    }

    pending_drop_event_ = event;
    pending_drop_allowed_ops_ = allowed_ops;
    pending_drop_active_ = true;

    host->DragTargetDragEnter(drag_data, event, allowed_ops);
    host->DragTargetDragOver(event, allowed_ops);

    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-drop-awaiting-acceptance"
                << " x=" << event.x << " y=" << event.y
                << " allowed=" << static_cast<int>(allowed_ops) << std::endl;
    }
    return true;
  }

  void GetViewRect(CefRefPtr<CefBrowser>, CefRect& rect) override {
    CEF_REQUIRE_UI_THREAD();
    std::scoped_lock lock(render_mutex_);
    rect = CefRect(0, 0, std::max(1, view_width_), std::max(1, view_height_));
  }

  bool GetScreenInfo(CefRefPtr<CefBrowser>,
                     CefScreenInfo& screen_info) override {
    CEF_REQUIRE_UI_THREAD();
    std::scoped_lock lock(render_mutex_);
    screen_info.device_scale_factor = scale_factor_;
    screen_info.rect =
        CefRect(0, 0, std::max(1, view_width_), std::max(1, view_height_));
    screen_info.available_rect = screen_info.rect;
    return true;
  }

  void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
                        DragOperation operation) override {
    CEF_REQUIRE_UI_THREAD();
    if (!pending_drop_active_ || !browser_ || !browser ||
        !browser_->IsSame(browser)) {
      return;
    }

    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-drop-cursor operation="
                << static_cast<int>(operation) << std::endl;
    }

    if (operation == DRAG_OPERATION_NONE) {
      return;
    }

    const auto allowed = static_cast<int>(pending_drop_allowed_ops_);
    const auto accepted = static_cast<int>(operation);
    auto host = browser_->GetHost();
    if (!host) {
      pending_drop_active_ = false;
      pending_drop_allowed_ops_ = DRAG_OPERATION_NONE;
      return;
    }

    if ((allowed & accepted) == 0) {
      host->DragTargetDragLeave();
      pending_drop_active_ = false;
      pending_drop_allowed_ops_ = DRAG_OPERATION_NONE;
      if (runtime_diagnostics_enabled()) {
        std::cerr << "[GoreeCloud CEF] windowless-drop-rejected operation="
                  << accepted << std::endl;
      }
      return;
    }

    const CefMouseEvent event = pending_drop_event_;
    pending_drop_active_ = false;
    pending_drop_allowed_ops_ = DRAG_OPERATION_NONE;

    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-drop-accepted operation="
                << accepted << std::endl;
    }
    host->DragTargetDrop(event);
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-drop-complete"
                << " x=" << event.x << " y=" << event.y << std::endl;
    }
  }

  bool OnCursorChange(CefRefPtr<CefBrowser>,
                      CefCursorHandle,
                      cef_cursor_type_t type,
                      const CefCursorInfo& custom_cursor_info) override {
    CEF_REQUIRE_UI_THREAD();

    NativeSurfaceCursorSink* sink = nullptr;
    {
      std::scoped_lock lock(render_mutex_);
      sink = cursor_sink_;
    }
    if (!sink) return false;

    if (type == CT_CUSTOM) {
      constexpr int kMaxCustomCursorDimension = 512;
      const int width = custom_cursor_info.size.width;
      const int height = custom_cursor_info.size.height;
      if (!custom_cursor_info.buffer || width <= 0 || height <= 0 ||
          width > kMaxCustomCursorDimension ||
          height > kMaxCustomCursorDimension) {
        if (runtime_diagnostics_enabled()) {
          std::cerr << "[GoreeCloud CEF] windowless-custom-cursor-rejected size="
                    << width << "x" << height << std::endl;
        }
        return false;
      }

      const auto byte_count =
          static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
      const auto* pixels =
          static_cast<const std::uint8_t*>(custom_cursor_info.buffer);

      NativeCustomCursor custom;
      custom.bgra.assign(pixels, pixels + byte_count);
      custom.width = width;
      custom.height = height;
      custom.hotspot_x =
          std::clamp(custom_cursor_info.hotspot.x, 0, width - 1);
      custom.hotspot_y =
          std::clamp(custom_cursor_info.hotspot.y, 0, height - 1);
      custom.scale_factor =
          std::isfinite(custom_cursor_info.image_scale_factor) &&
                  custom_cursor_info.image_scale_factor > 0.0F
              ? std::clamp(custom_cursor_info.image_scale_factor, 0.25F, 8.0F)
              : 1.0F;

      sink->apply_custom_cursor(std::move(custom));
      if (runtime_diagnostics_enabled()) {
        std::cerr << "[GoreeCloud CEF] windowless-custom-cursor-change size="
                  << width << "x" << height
                  << " hotspot=" << custom_cursor_info.hotspot.x
                  << "," << custom_cursor_info.hotspot.y
                  << " scale=" << custom_cursor_info.image_scale_factor
                  << std::endl;
      }
      return true;
    }

    const auto mapped = native_cursor_type(type);
    if (!mapped) {
      if (runtime_diagnostics_enabled()) {
        std::cerr << "[GoreeCloud CEF] windowless-cursor-unsupported raw="
                  << static_cast<int>(type) << std::endl;
      }
      return false;
    }

    sink->apply_native_cursor(*mapped);
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-cursor-change type="
                << native_cursor_name(*mapped)
                << " raw=" << static_cast<int>(type) << std::endl;
    }
    return true;
  }

  void OnPopupShow(CefRefPtr<CefBrowser>, bool show) override {
    CEF_REQUIRE_UI_THREAD();

    NativeSurfaceFrameSink* sink = nullptr;
    {
      std::scoped_lock lock(render_mutex_);
      sink = frame_sink_;
      popup_visible_ = show;
      if (!show) popup_rect_ = CefRect();
    }
    if (!show && sink) sink->clear_software_popup_frame();

    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-popup-"
                << (show ? "shown" : "hidden") << std::endl;
    }
  }

  void OnPopupSize(CefRefPtr<CefBrowser>, const CefRect& rect) override {
    CEF_REQUIRE_UI_THREAD();
    {
      std::scoped_lock lock(render_mutex_);
      popup_rect_ = rect;
    }
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-popup-size x=" << rect.x
                << " y=" << rect.y << " width=" << rect.width
                << " height=" << rect.height << std::endl;
    }
  }

  void OnPaint(CefRefPtr<CefBrowser>,
               PaintElementType type,
               const RectList&,
               const void* buffer,
               int width,
               int height) override {
    CEF_REQUIRE_UI_THREAD();
    if (!buffer || width <= 0 || height <= 0) return;

    NativeSurfaceFrameSink* sink = nullptr;
    float scale_factor = 1.0F;
    CefRect popup_rect;
    bool popup_visible = false;
    {
      std::scoped_lock lock(render_mutex_);
      sink = frame_sink_;
      scale_factor = scale_factor_;
      popup_rect = popup_rect_;
      popup_visible = popup_visible_;
    }
    if (!sink) return;

    const NativeSurfaceFrame frame{
        .bgra = static_cast<const std::uint8_t*>(buffer),
        .width = width,
        .height = height,
        .stride = width * 4,
        .scale_factor = scale_factor};

    if (type == PET_VIEW) {
      sink->present_software_frame(frame);
      return;
    }

    if (type == PET_POPUP && popup_visible && popup_rect.width > 0 &&
        popup_rect.height > 0) {
      sink->present_software_popup_frame(frame, popup_rect.x, popup_rect.y);
      if (runtime_diagnostics_enabled()) {
        std::cerr << "[GoreeCloud CEF] windowless-popup-frame size="
                  << width << "x" << height << " x=" << popup_rect.x
                  << " y=" << popup_rect.y << std::endl;
      }
    }
  }

  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
    CEF_REQUIRE_UI_THREAD();
    browser_ = browser;
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] OnAfterCreated browser_id="
                << browser->GetIdentifier() << std::endl;
    }
    // Browser creation itself does not represent a navigation-state change.
    // Publishing the client's default state here can transiently erase the
    // initial URL and cause the native host to detach/re-attach the same view.
  }

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
    CEF_REQUIRE_UI_THREAD();
    if (browser_ && browser_->IsSame(browser)) browser_ = nullptr;
    fail_all_pending_probes();
    fail_all_pending_previews("Browser closed before preview completed.");
    if (closed_callback_) closed_callback_();
  }

  void OnTitleChange(CefRefPtr<CefBrowser>, const CefString& title) override {
    CEF_REQUIRE_UI_THREAD();
    state_.title = title.ToString();
    publish();
  }

  void OnLoadingStateChange(CefRefPtr<CefBrowser>,
                            bool is_loading,
                            bool can_go_back,
                            bool can_go_forward) override {
    CEF_REQUIRE_UI_THREAD();
    state_.loading = is_loading;
    state_.can_go_back = can_go_back;
    state_.can_go_forward = can_go_forward;
    if (!is_loading) state_.progress = 1.0;
    publish();
  }

  void OnLoadStart(CefRefPtr<CefBrowser>,
                   CefRefPtr<CefFrame> frame,
                   TransitionType) override {
    CEF_REQUIRE_UI_THREAD();
    if (!frame || !frame->IsMain()) return;
    state_.url = frame->GetURL().ToString();
    state_.loading = true;
    state_.progress = 0.0;
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] main-frame-load-start url="
                << state_.url << std::endl;
    }
    publish();
  }

  void OnLoadEnd(CefRefPtr<CefBrowser>,
                 CefRefPtr<CefFrame> frame,
                 int http_status_code) override {
    CEF_REQUIRE_UI_THREAD();
    if (!frame || !frame->IsMain()) return;
    state_.url = frame->GetURL().ToString();
    state_.loading = false;
    state_.progress = 1.0;
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] main-frame-load-end status="
                << http_status_code
                << " url=" << state_.url << std::endl;
    }
    publish();
  }

  void OnLoadError(CefRefPtr<CefBrowser>,
                   CefRefPtr<CefFrame> frame,
                   ErrorCode error_code,
                   const CefString& error_text,
                   const CefString& failed_url) override {
    CEF_REQUIRE_UI_THREAD();
    if (!frame || !frame->IsMain()) return;
    state_.url = failed_url.ToString();
    state_.loading = false;
    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] main-frame-load-error code="
                << static_cast<int>(error_code)
                << " text=" << error_text.ToString()
                << " url=" << state_.url << std::endl;
    }
    publish();
  }

  bool request_media_probe(int viewport_x,
                           int viewport_y,
                           std::uint64_t sequence,
                           MediaProbeCallback callback) {
    CEF_REQUIRE_UI_THREAD();
    if (!browser_ || !callback) return false;
    auto frame = browser_->GetMainFrame();
    if (!frame) return false;

    {
      std::scoped_lock lock(media_probe_mutex_);
      pending_media_probes_[sequence] = std::move(callback);
    }

    auto message = CefProcessMessage::Create(kMediaProbeRequestMessage);
    auto args = message->GetArgumentList();
    args->SetDouble(0, static_cast<double>(sequence));
    args->SetInt(1, viewport_x);
    args->SetInt(2, viewport_y);
    frame->SendProcessMessage(PID_RENDERER, message);
    return true;
  }

  bool request_media_preview(const MediaPreviewRequest& request,
                             MediaPreviewCallback callback) {
    CEF_REQUIRE_UI_THREAD();
    if (!browser_ || !callback || request.target.media_url.empty()) return false;
    if (request.target.protected_media) {
      callback(std::nullopt, "Protected media preview is not permitted.");
      return false;
    }
    auto frame = browser_->GetMainFrame();
    if (!frame) return false;

    const auto request_id = ++next_preview_request_id_;
    {
      std::scoped_lock lock(media_preview_mutex_);
      pending_media_previews_[request_id] = std::move(callback);
    }

    auto message = CefProcessMessage::Create(kMediaPreviewRequestMessage);
    auto args = message->GetArgumentList();
    args->SetDouble(0, static_cast<double>(request_id));
    args->SetString(1, request.target.media_url);
    args->SetString(2, request.target.kind == MediaKind::video ? "video" : "image");
    args->SetInt(3, request.maximum_width);
    args->SetInt(4, request.maximum_height);

    frame->SendProcessMessage(PID_RENDERER, message);
    return true;
  }

  bool OnProcessMessageReceived(CefRefPtr<CefBrowser>,
                                CefRefPtr<CefFrame>,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) override {
    CEF_REQUIRE_UI_THREAD();
    if (source_process != PID_RENDERER || !message) return false;
    if (message->GetName() == kMediaProbeResponseMessage) {
      return handle_media_probe_response(message);
    }
    if (message->GetName() == kMediaPreviewResponseMessage) {
      return handle_media_preview_response(message);
    }
    return false;
  }

  bool RunContextMenu(CefRefPtr<CefBrowser>,
                      CefRefPtr<CefFrame>,
                      CefRefPtr<CefContextMenuParams> params,
                      CefRefPtr<CefMenuModel> model,
                      CefRefPtr<CefRunContextMenuCallback> callback) override {
    CEF_REQUIRE_UI_THREAD();
    if (!params || !model || !callback) return false;

    NativeSurfaceContextMenuSink* sink = nullptr;
    {
      std::scoped_lock lock(render_mutex_);
      sink = context_menu_sink_;
    }
    if (!sink) return false;

    NativeContextMenuRequest request;
    request.x = params->GetXCoord();
    request.y = params->GetYCoord();
    append_context_menu_items(model, request.items);

    if (request.items.empty()) {
      callback->Cancel();
      return true;
    }

    if (runtime_diagnostics_enabled()) {
      std::cerr << "[GoreeCloud CEF] windowless-context-menu-request items="
                << request.items.size()
                << " x=" << request.x << " y=" << request.y << std::endl;
    }

    sink->show_native_context_menu(
        std::move(request),
        [callback](std::optional<int> command_id) {
          if (runtime_diagnostics_enabled()) {
            if (command_id) {
              std::cerr << "[GoreeCloud CEF] windowless-context-menu-result command="
                        << *command_id << std::endl;
            } else {
              std::cerr << "[GoreeCloud CEF] windowless-context-menu-result canceled"
                        << std::endl;
            }
          }
          if (command_id) callback->Continue(*command_id, EVENTFLAG_NONE);
          else callback->Cancel();
        });
    return true;
  }

  void OnBeforeContextMenu(CefRefPtr<CefBrowser>,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefContextMenuParams> params,
                           CefRefPtr<CefMenuModel>) override {
    CEF_REQUIRE_UI_THREAD();
    if (!params) return;

    RawMediaHitTest hit;
    hit.page_url = frame ? frame->GetURL().ToString() : state_.url;
    hit.media_url = params->GetSourceUrl().ToString();
    hit.link_url = params->GetLinkUrl().ToString();
    // Context-menu parameters do not expose source MIME in current CEF.
    // The asynchronous renderer probe supplies MIME when available.
    hit.mime_type.clear();
    hit.alt_text = params->GetTitleText().ToString();

    const auto media_type = params->GetMediaType();
    switch (media_type) {
      case CM_MEDIATYPE_IMAGE:
        hit.kind = MediaKind::image;
        hit.copyable = true;
        hit.downloadable = !hit.media_url.empty();
        hit.region_selectable = true;
        hit.ocr_supported = true;
        break;
      case CM_MEDIATYPE_VIDEO:
        hit.kind = MediaKind::video;
        hit.copyable = !hit.media_url.empty();
        hit.downloadable = !hit.media_url.empty();
        hit.frame_capture_supported = true;
        hit.region_selectable = true;
        hit.ocr_supported = true;
        break;
      default:
        hit.kind = MediaKind::unknown;
        break;
    }

    const auto flags = params->GetTypeFlags();
    hit.linked = (flags & CM_TYPEFLAG_LINK) != 0 && !hit.link_url.empty();
    hit.secure_resource = hit.media_url.rfind("https://", 0) == 0;

    {
      std::scoped_lock lock(media_mutex_);
      last_media_context_ = hit;
    }
    if (media_context_callback_) media_context_callback_(hit);
  }

  [[nodiscard]] CefRefPtr<CefBrowser> browser() const { return browser_; }

  [[nodiscard]] std::optional<RawMediaHitTest> last_media_context() const {
    std::scoped_lock lock(media_mutex_);
    return last_media_context_;
  }

 private:
  static std::optional<NativeCursorType> native_cursor_type(
      cef_cursor_type_t type) {
    switch (type) {
      case CT_POINTER:
        return NativeCursorType::pointer;
      case CT_CROSS:
        return NativeCursorType::crosshair;
      case CT_HAND:
        return NativeCursorType::hand;
      case CT_IBEAM:
        return NativeCursorType::text;
      case CT_WAIT:
        return NativeCursorType::wait;
      case CT_HELP:
        return NativeCursorType::help;
      case CT_EASTRESIZE:
      case CT_WESTRESIZE:
      case CT_EASTWESTRESIZE:
        return NativeCursorType::east_west_resize;
      case CT_NORTHRESIZE:
      case CT_SOUTHRESIZE:
      case CT_NORTHSOUTHRESIZE:
        return NativeCursorType::north_south_resize;
      case CT_NORTHEASTRESIZE:
      case CT_SOUTHWESTRESIZE:
      case CT_NORTHEASTSOUTHWESTRESIZE:
        return NativeCursorType::northeast_southwest_resize;
      case CT_NORTHWESTRESIZE:
      case CT_SOUTHEASTRESIZE:
      case CT_NORTHWESTSOUTHEASTRESIZE:
        return NativeCursorType::northwest_southeast_resize;
      case CT_COLUMNRESIZE:
        return NativeCursorType::column_resize;
      case CT_ROWRESIZE:
        return NativeCursorType::row_resize;
      case CT_MIDDLEPANNING:
      case CT_EASTPANNING:
      case CT_NORTHPANNING:
      case CT_NORTHEASTPANNING:
      case CT_NORTHWESTPANNING:
      case CT_SOUTHPANNING:
      case CT_SOUTHEASTPANNING:
      case CT_SOUTHWESTPANNING:
      case CT_WESTPANNING:
      case CT_MOVE:
        return NativeCursorType::move;
      case CT_VERTICALTEXT:
        return NativeCursorType::vertical_text;
      case CT_CELL:
        return NativeCursorType::cell;
      case CT_CONTEXTMENU:
        return NativeCursorType::context_menu;
      case CT_ALIAS:
        return NativeCursorType::alias;
      case CT_PROGRESS:
        return NativeCursorType::progress;
      case CT_NODROP:
        return NativeCursorType::no_drop;
      case CT_COPY:
        return NativeCursorType::copy;
      case CT_NONE:
        return NativeCursorType::none;
      case CT_NOTALLOWED:
        return NativeCursorType::not_allowed;
      case CT_ZOOMIN:
        return NativeCursorType::zoom_in;
      case CT_ZOOMOUT:
        return NativeCursorType::zoom_out;
      case CT_GRAB:
        return NativeCursorType::grab;
      case CT_GRABBING:
        return NativeCursorType::grabbing;
      case CT_CUSTOM:
      default:
        return std::nullopt;
    }
  }

  static const char* native_cursor_name(NativeCursorType cursor) {
    switch (cursor) {
      case NativeCursorType::pointer: return "pointer";
      case NativeCursorType::crosshair: return "crosshair";
      case NativeCursorType::hand: return "hand";
      case NativeCursorType::text: return "text";
      case NativeCursorType::wait: return "wait";
      case NativeCursorType::help: return "help";
      case NativeCursorType::move: return "move";
      case NativeCursorType::east_west_resize: return "east-west-resize";
      case NativeCursorType::north_south_resize: return "north-south-resize";
      case NativeCursorType::northeast_southwest_resize:
        return "northeast-southwest-resize";
      case NativeCursorType::northwest_southeast_resize:
        return "northwest-southeast-resize";
      case NativeCursorType::column_resize: return "column-resize";
      case NativeCursorType::row_resize: return "row-resize";
      case NativeCursorType::vertical_text: return "vertical-text";
      case NativeCursorType::cell: return "cell";
      case NativeCursorType::context_menu: return "context-menu";
      case NativeCursorType::alias: return "alias";
      case NativeCursorType::progress: return "progress";
      case NativeCursorType::no_drop: return "no-drop";
      case NativeCursorType::copy: return "copy";
      case NativeCursorType::none: return "none";
      case NativeCursorType::not_allowed: return "not-allowed";
      case NativeCursorType::zoom_in: return "zoom-in";
      case NativeCursorType::zoom_out: return "zoom-out";
      case NativeCursorType::grab: return "grab";
      case NativeCursorType::grabbing: return "grabbing";
    }
    return "pointer";
  }

  static void append_context_menu_items(
      CefRefPtr<CefMenuModel> model,
      std::vector<NativeContextMenuItem>& items) {
    if (!model) return;
    const auto count = model->GetCount();
    for (std::size_t index = 0; index < count; ++index) {
      NativeContextMenuItem item;
      const auto type = model->GetTypeAt(index);
      switch (type) {
        case MENUITEMTYPE_SEPARATOR:
          item.type = NativeContextMenuItemType::separator;
          break;
        case MENUITEMTYPE_CHECK:
          item.type = NativeContextMenuItemType::check;
          break;
        case MENUITEMTYPE_RADIO:
          item.type = NativeContextMenuItemType::radio;
          break;
        case MENUITEMTYPE_SUBMENU:
          item.type = NativeContextMenuItemType::submenu;
          break;
        case MENUITEMTYPE_COMMAND:
          item.type = NativeContextMenuItemType::command;
          break;
        default:
          continue;
      }

      if (item.type != NativeContextMenuItemType::separator) {
        item.command_id = model->GetCommandIdAt(index);
        item.label = model->GetLabelAt(index).ToString();
        item.enabled = model->IsEnabledAt(index);
        item.checked = model->IsCheckedAt(index);
      }
      if (item.type == NativeContextMenuItemType::submenu) {
        append_context_menu_items(model->GetSubMenuAt(index), item.children);
      }
      items.push_back(std::move(item));
    }
  }

  static bool runtime_diagnostics_enabled() {
    const char* diagnostics =
        std::getenv("GOREECLOUD_BROWSER_RUNTIME_DIAGNOSTICS");
    return diagnostics && *diagnostics &&
           std::string_view{diagnostics} != "0";
  }

  bool handle_media_probe_response(CefRefPtr<CefProcessMessage> message) {
    auto args = message->GetArgumentList();
    if (!args || args->GetSize() < 2) return true;

    const auto sequence = static_cast<std::uint64_t>(args->GetDouble(0));
    MediaProbeCallback callback;
    {
      std::scoped_lock lock(media_probe_mutex_);
      const auto it = pending_media_probes_.find(sequence);
      if (it == pending_media_probes_.end()) return true;
      callback = std::move(it->second);
      pending_media_probes_.erase(it);
    }

    if (!callback) return true;
    if (!args->GetBool(1) || args->GetSize() < 16) {
      callback(sequence, std::nullopt);
      return true;
    }

    RawMediaHitTest hit;
    const auto kind = args->GetString(2).ToString();
    if (kind == "image") hit.kind = MediaKind::image;
    else if (kind == "video") hit.kind = MediaKind::video;
    else if (kind == "background_image") hit.kind = MediaKind::background_image;
    else hit.kind = MediaKind::unknown;

    hit.page_url = args->GetString(3).ToString();
    hit.media_url = args->GetString(4).ToString();
    hit.link_url = args->GetString(5).ToString();
    hit.mime_type = args->GetString(6).ToString();
    hit.alt_text = args->GetString(7).ToString();
    hit.displayed_width = args->GetInt(8);
    hit.displayed_height = args->GetInt(9);
    hit.intrinsic_width = args->GetInt(10);
    hit.intrinsic_height = args->GetInt(11);
    hit.duration_seconds = args->GetDouble(12);
    hit.secure_resource = args->GetBool(13);
    hit.cross_origin = args->GetBool(14);
    hit.animated = args->GetBool(15);
    hit.linked = !hit.link_url.empty();

    switch (hit.kind) {
      case MediaKind::image:
      case MediaKind::animated_image:
      case MediaKind::background_image:
        hit.copyable = true;
        hit.downloadable = !hit.media_url.empty();
        hit.region_selectable = true;
        hit.ocr_supported = true;
        break;
      case MediaKind::video:
        hit.copyable = !hit.media_url.empty();
        hit.downloadable = !hit.media_url.empty();
        hit.frame_capture_supported = true;
        hit.region_selectable = true;
        hit.ocr_supported = true;
        break;
      default:
        break;
    }

    callback(sequence, hit.kind == MediaKind::unknown || hit.media_url.empty()
                           ? std::nullopt
                           : std::optional<RawMediaHitTest>{std::move(hit)});
    return true;
  }

  bool handle_media_preview_response(CefRefPtr<CefProcessMessage> message) {
    auto args = message->GetArgumentList();
    if (!args || args->GetSize() < 2) return true;
    const auto request_id = static_cast<std::uint64_t>(args->GetDouble(0));

    MediaPreviewCallback callback;
    {
      std::scoped_lock lock(media_preview_mutex_);
      const auto it = pending_media_previews_.find(request_id);
      if (it == pending_media_previews_.end()) return true;
      callback = std::move(it->second);
      pending_media_previews_.erase(it);
    }
    if (!callback) return true;

    if (!args->GetBool(1)) {
      const std::string error = args->GetSize() > 2
                                    ? args->GetString(2).ToString()
                                    : "Media preview failed.";
      callback(std::nullopt, error);
      return true;
    }
    if (args->GetSize() < 6) {
      callback(std::nullopt, "Media preview response was incomplete.");
      return true;
    }

    const std::string data_url = args->GetString(5).ToString();
    constexpr const char kPrefix[] = "data:image/png;base64,";
    if (data_url.rfind(kPrefix, 0) != 0) {
      callback(std::nullopt, "Media preview returned an unsupported encoding.");
      return true;
    }

    auto decoded = decode_base64(data_url.substr(sizeof(kPrefix) - 1));
    if (!decoded || decoded->empty()) {
      callback(std::nullopt, "Media preview image could not be decoded.");
      return true;
    }

    MediaPreviewFrame frame;
    frame.width = args->GetInt(2);
    frame.height = args->GetInt(3);
    frame.mime_type = args->GetString(4).ToString();
    frame.encoded_bytes = std::move(*decoded);
    callback(std::move(frame), {});
    return true;
  }

  static std::optional<std::vector<std::uint8_t>> decode_base64(const std::string& input) {
    static constexpr unsigned char kInvalid = 0xFF;
    static constexpr char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char table[256];
    for (auto& value : table) value = kInvalid;
    for (unsigned int i = 0; i < 64; ++i) {
      table[static_cast<unsigned char>(kAlphabet[i])] = static_cast<unsigned char>(i);
    }

    std::vector<std::uint8_t> output;
    int accumulator = 0;
    int bits = -8;
    for (const unsigned char ch : input) {
      if (ch == '=') break;
      const auto value = table[ch];
      if (value == kInvalid) return std::nullopt;
      accumulator = (accumulator << 6) | value;
      bits += 6;
      if (bits >= 0) {
        output.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xFF));
        bits -= 8;
      }
    }
    return output;
  }

  void publish() {
    if (navigation_callback_) navigation_callback_(state_);
  }

  void fail_all_pending_probes() {
    std::unordered_map<std::uint64_t, MediaProbeCallback> pending;
    {
      std::scoped_lock lock(media_probe_mutex_);
      pending.swap(pending_media_probes_);
    }
    for (auto& [sequence, callback] : pending) {
      if (callback) callback(sequence, std::nullopt);
    }
  }

  void fail_all_pending_previews(const std::string& reason) {
    std::unordered_map<std::uint64_t, MediaPreviewCallback> pending;
    {
      std::scoped_lock lock(media_preview_mutex_);
      pending.swap(pending_media_previews_);
    }
    for (auto& [_, callback] : pending) {
      if (callback) callback(std::nullopt, reason);
    }
  }

  CefRefPtr<CefBrowser> browser_;
  NavigationState state_;
  mutable std::mutex render_mutex_;
  NativeSurfaceFrameSink* frame_sink_{nullptr};
  NativeSurfaceCursorSink* cursor_sink_{nullptr};
  NativeSurfaceContextMenuSink* context_menu_sink_{nullptr};
  CefRect popup_rect_;
  bool popup_visible_{false};
  CefMouseEvent pending_drop_event_;
  CefBrowserHost::DragOperationsMask pending_drop_allowed_ops_{
      DRAG_OPERATION_NONE};
  bool pending_drop_active_{false};
  int view_width_{1};
  int view_height_{1};
  float scale_factor_{1.0F};
  NavigationCallback navigation_callback_;
  ClosedCallback closed_callback_;
  MediaContextCallback media_context_callback_;
  mutable std::mutex media_mutex_;
  std::optional<RawMediaHitTest> last_media_context_;
  std::mutex media_probe_mutex_;
  std::unordered_map<std::uint64_t, MediaProbeCallback> pending_media_probes_;
  std::mutex media_preview_mutex_;
  std::unordered_map<std::uint64_t, MediaPreviewCallback> pending_media_previews_;
  std::uint64_t next_preview_request_id_{0};

  IMPLEMENT_REFCOUNTING(GoreeCloudCefClient);
  DISALLOW_COPY_AND_ASSIGN(GoreeCloudCefClient);
};

#endif

}  // namespace goreecloud::browser
