#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "goreecloud/browser/cef_media_probe.hpp"

#if GOREECLOUD_ENABLE_CEF
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/cef_process_message.h"
#include "include/cef_v8.h"
#endif

namespace goreecloud::browser {

#if GOREECLOUD_ENABLE_CEF

class GoreeCloudCefBrowserApp final : public CefApp,
                                      public CefBrowserProcessHandler {
 public:
  GoreeCloudCefBrowserApp() = default;

  CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override {
    return this;
  }

  void OnContextInitialized() override {
    browser_context_initialized_.store(true, std::memory_order_release);
  }

  [[nodiscard]] bool browser_context_initialized() const noexcept {
    return browser_context_initialized_.load(std::memory_order_acquire);
  }

 private:
  std::atomic_bool browser_context_initialized_{false};

  IMPLEMENT_REFCOUNTING(GoreeCloudCefBrowserApp);
  DISALLOW_COPY_AND_ASSIGN(GoreeCloudCefBrowserApp);
};

class GoreeCloudCefRenderApp final : public CefApp,
                                     public CefRenderProcessHandler {
 public:
  GoreeCloudCefRenderApp() = default;

  CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
    return this;
  }

  bool OnProcessMessageReceived(CefRefPtr<CefBrowser>,
                                CefRefPtr<CefFrame> frame,
                                CefProcessId source_process,
                                CefRefPtr<CefProcessMessage> message) override {
    if (source_process != PID_BROWSER || !message || !frame) return false;
    if (message->GetName() == kMediaProbeRequestMessage) {
      return handle_probe(frame, message);
    }
    if (message->GetName() == kMediaPreviewRequestMessage) {
      return handle_preview(frame, message);
    }
    return false;
  }

 private:
  static bool handle_probe(CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefProcessMessage> message) {
    auto args = message->GetArgumentList();
    if (!args || args->GetSize() < 3) return true;

    const auto sequence = static_cast<std::uint64_t>(args->GetDouble(0));
    const int x = args->GetInt(1);
    const int y = args->GetInt(2);

    auto context = frame->GetV8Context();
    if (!context || !context->Enter()) {
      send_empty_probe(frame, sequence);
      return true;
    }

    static constexpr const char* kProbeScript = R"JS((function(x,y){
      const el = document.elementFromPoint(x,y);
      if (!el) return null;
      const closest = (node, selector) => node && node.closest ? node.closest(selector) : null;
      const img = closest(el, 'img');
      const video = closest(el, 'video');
      const media = img || video || el;
      const link = closest(el, 'a[href]');
      const style = window.getComputedStyle(media);
      let bg = '';
      if (!img && !video && style && style.backgroundImage && style.backgroundImage !== 'none') {
        const match = style.backgroundImage.match(/^url\(["']?(.*?)["']?\)$/);
        if (match) bg = match[1];
      }
      let kind = '';
      let src = '';
      let mime = '';
      let alt = '';
      let intrinsicWidth = 0;
      let intrinsicHeight = 0;
      let duration = 0;
      let animated = false;
      if (img) {
        kind = 'image';
        src = img.currentSrc || img.src || '';
        alt = img.alt || img.title || '';
        intrinsicWidth = img.naturalWidth || 0;
        intrinsicHeight = img.naturalHeight || 0;
      } else if (video) {
        kind = 'video';
        src = video.currentSrc || video.src || '';
        intrinsicWidth = video.videoWidth || 0;
        intrinsicHeight = video.videoHeight || 0;
        duration = Number.isFinite(video.duration) ? video.duration : 0;
      } else if (bg) {
        kind = 'background_image';
        src = bg;
      } else {
        return null;
      }
      const rect = media.getBoundingClientRect();
      return {
        kind, src, link: link ? link.href : '', page: location.href,
        mime, alt, animated,
        displayedWidth: Math.max(0, Math.round(rect.width)),
        displayedHeight: Math.max(0, Math.round(rect.height)),
        intrinsicWidth, intrinsicHeight, duration,
        secure: location.protocol === 'https:' && (!src || src.startsWith('https:') || src.startsWith('data:') || src.startsWith('blob:')),
        crossOrigin: !!src && (function(){ try { return new URL(src, location.href).origin !== location.origin; } catch (_) { return false; } })()
      };
    }))JS";

    CefRefPtr<CefV8Value> retval;
    CefRefPtr<CefV8Exception> exception;
    const std::string script = std::string{"("} + kProbeScript + ")(" +
                               std::to_string(x) + "," + std::to_string(y) + ")";
    const bool ok = context->Eval(script, frame->GetURL(), 0, retval, exception);
    context->Exit();

    if (!ok || !retval || retval->IsNull() || !retval->IsObject()) {
      send_empty_probe(frame, sequence);
      return true;
    }

    auto response = CefProcessMessage::Create(kMediaProbeResponseMessage);
    auto out = response->GetArgumentList();
    out->SetDouble(0, static_cast<double>(sequence));
    out->SetBool(1, true);
    out->SetString(2, get_string(retval, "kind"));
    out->SetString(3, get_string(retval, "page"));
    out->SetString(4, get_string(retval, "src"));
    out->SetString(5, get_string(retval, "link"));
    out->SetString(6, get_string(retval, "mime"));
    out->SetString(7, get_string(retval, "alt"));
    out->SetInt(8, get_int(retval, "displayedWidth"));
    out->SetInt(9, get_int(retval, "displayedHeight"));
    out->SetInt(10, get_int(retval, "intrinsicWidth"));
    out->SetInt(11, get_int(retval, "intrinsicHeight"));
    out->SetDouble(12, get_double(retval, "duration"));
    out->SetBool(13, get_bool(retval, "secure"));
    out->SetBool(14, get_bool(retval, "crossOrigin"));
    out->SetBool(15, get_bool(retval, "animated"));
    frame->SendProcessMessage(PID_BROWSER, response);
    return true;
  }

  static bool handle_preview(CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefProcessMessage> message) {
    auto args = message->GetArgumentList();
    if (!args || args->GetSize() < 5) return true;

    const auto request_id = static_cast<std::uint64_t>(args->GetDouble(0));
    const std::string media_url = args->GetString(1).ToString();
    const std::string kind = args->GetString(2).ToString();
    const int maximum_width = args->GetInt(3);
    const int maximum_height = args->GetInt(4);

    auto context = frame->GetV8Context();
    if (!context || !context->Enter()) {
      send_preview_error(frame, request_id, "Renderer context unavailable.");
      return true;
    }

    static constexpr const char* kPreviewFunction = R"JS((function(url,kind,maxW,maxH){
      const absolute = (value) => { try { return new URL(value, location.href).href; } catch (_) { return value || ''; } };
      let media = null;
      if (kind === 'video') {
        for (const candidate of document.querySelectorAll('video')) {
          if (absolute(candidate.currentSrc || candidate.src) === absolute(url)) { media = candidate; break; }
        }
        if (!media || media.readyState < 2 || !media.videoWidth || !media.videoHeight) return {error:'Video frame is not currently capturable.'};
      } else {
        for (const candidate of document.querySelectorAll('img')) {
          if (absolute(candidate.currentSrc || candidate.src) === absolute(url)) { media = candidate; break; }
        }
        if (!media || !media.complete || !media.naturalWidth || !media.naturalHeight) return {error:'Image is not currently available for preview.'};
      }
      const sourceW = kind === 'video' ? media.videoWidth : media.naturalWidth;
      const sourceH = kind === 'video' ? media.videoHeight : media.naturalHeight;
      const scale = Math.min(1, maxW / sourceW, maxH / sourceH);
      const width = Math.max(1, Math.round(sourceW * scale));
      const height = Math.max(1, Math.round(sourceH * scale));
      const canvas = document.createElement('canvas');
      canvas.width = width;
      canvas.height = height;
      const ctx = canvas.getContext('2d', {alpha:true});
      if (!ctx) return {error:'Canvas preview is unavailable.'};
      try {
        ctx.drawImage(media, 0, 0, width, height);
        const data = canvas.toDataURL('image/png');
        if (!data || !data.startsWith('data:image/png;base64,')) return {error:'Preview encoding failed.'};
        return {width, height, data};
      } catch (_) {
        return {error:'Engine security policy prevented media preview capture.'};
      }
    }))JS";

    CefRefPtr<CefV8Value> retval;
    CefRefPtr<CefV8Exception> exception;
    const std::string escaped_url = js_string(media_url);
    const std::string escaped_kind = js_string(kind);
    const std::string script = std::string{"("} + kPreviewFunction + ")(" +
                               escaped_url + "," + escaped_kind + "," +
                               std::to_string(maximum_width) + "," +
                               std::to_string(maximum_height) + ")";
    const bool ok = context->Eval(script, frame->GetURL(), 0, retval, exception);
    context->Exit();

    if (!ok || !retval || !retval->IsObject()) {
      send_preview_error(frame, request_id, "Preview evaluation failed.");
      return true;
    }
    const auto error = get_string(retval, "error");
    if (!error.empty()) {
      send_preview_error(frame, request_id, error);
      return true;
    }

    auto response = CefProcessMessage::Create(kMediaPreviewResponseMessage);
    auto out = response->GetArgumentList();
    out->SetDouble(0, static_cast<double>(request_id));
    out->SetBool(1, true);
    out->SetInt(2, get_int(retval, "width"));
    out->SetInt(3, get_int(retval, "height"));
    out->SetString(4, "image/png");
    out->SetString(5, get_string(retval, "data"));
    frame->SendProcessMessage(PID_BROWSER, response);
    return true;
  }

  static std::string js_string(const std::string& value) {
    std::string out{"\""};
    for (const char ch : value) {
      switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
      }
    }
    out += '"';
    return out;
  }

  static std::string get_string(CefRefPtr<CefV8Value> object, const char* key) {
    auto value = object->GetValue(key);
    return value && value->IsString() ? value->GetStringValue().ToString() : std::string{};
  }
  static int get_int(CefRefPtr<CefV8Value> object, const char* key) {
    auto value = object->GetValue(key);
    return value && value->IsInt() ? value->GetIntValue() : 0;
  }
  static double get_double(CefRefPtr<CefV8Value> object, const char* key) {
    auto value = object->GetValue(key);
    return value && (value->IsDouble() || value->IsInt()) ? value->GetDoubleValue() : 0.0;
  }
  static bool get_bool(CefRefPtr<CefV8Value> object, const char* key) {
    auto value = object->GetValue(key);
    return value && value->IsBool() ? value->GetBoolValue() : false;
  }

  static void send_empty_probe(CefRefPtr<CefFrame> frame, std::uint64_t sequence) {
    auto response = CefProcessMessage::Create(kMediaProbeResponseMessage);
    auto out = response->GetArgumentList();
    out->SetDouble(0, static_cast<double>(sequence));
    out->SetBool(1, false);
    frame->SendProcessMessage(PID_BROWSER, response);
  }

  static void send_preview_error(CefRefPtr<CefFrame> frame,
                                 std::uint64_t request_id,
                                 const std::string& error) {
    auto response = CefProcessMessage::Create(kMediaPreviewResponseMessage);
    auto out = response->GetArgumentList();
    out->SetDouble(0, static_cast<double>(request_id));
    out->SetBool(1, false);
    out->SetString(2, error);
    frame->SendProcessMessage(PID_BROWSER, response);
  }

  IMPLEMENT_REFCOUNTING(GoreeCloudCefRenderApp);
  DISALLOW_COPY_AND_ASSIGN(GoreeCloudCefRenderApp);
};

#endif

}  // namespace goreecloud::browser
