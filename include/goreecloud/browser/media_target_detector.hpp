#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "goreecloud/browser/media_hover.hpp"

namespace goreecloud::browser {

enum class EngineMediaElementKind {
  none,
  image,
  video,
  animated_image,
  thumbnail,
  background_image,
  embedded_media,
};

struct RawMediaHitTest {
  MediaKind kind{MediaKind::unknown};
  std::string page_url;
  std::string media_url;
  std::string link_url;
  std::string mime_type;
  std::string alt_text;
  int intrinsic_width{0};
  int intrinsic_height{0};
  int displayed_width{0};
  int displayed_height{0};
  double duration_seconds{0.0};
  bool animated{false};
  bool secure_resource{false};
  bool downloadable{false};
  bool copyable{false};
  bool frame_capture_supported{false};
  bool protected_media{false};
  bool drm_protected{false};
  bool cross_origin{false};
  bool linked{false};
  bool region_selectable{false};
  bool ocr_supported{false};
};

struct EngineMediaHitTest {
  EngineMediaElementKind kind{EngineMediaElementKind::none};
  std::string page_url;
  std::string page_title;
  std::string media_url;
  std::optional<std::string> link_url;
  std::string mime_type;
  std::string alt_text;
  int intrinsic_width{0};
  int intrinsic_height{0};
  int displayed_width{0};
  int displayed_height{0};
  double duration_seconds{0.0};
  bool animated{false};
  bool secure_resource{false};
  bool downloadable{false};
  bool copyable{false};
  bool capturable_frame{false};
  bool protected_media{false};
  bool drm_protected{false};
  bool same_origin_with_page{false};
};

class MediaTargetDetector {
 public:
  [[nodiscard]] static std::optional<MediaTarget> normalize(
      const EngineMediaHitTest& hit) {
    if (hit.kind == EngineMediaElementKind::none || hit.media_url.empty()) {
      return std::nullopt;
    }

    MediaTarget target;
    target.kind = to_media_kind(hit.kind);
    target.page_url = hit.page_url;
    target.page_title = hit.page_title;
    target.media_url = hit.media_url;
    target.link_url = hit.link_url;
    target.mime_type = hit.mime_type;
    target.alt_text = hit.alt_text;
    target.intrinsic_width = hit.intrinsic_width;
    target.intrinsic_height = hit.intrinsic_height;
    target.displayed_width = hit.displayed_width;
    target.displayed_height = hit.displayed_height;
    target.duration_seconds = hit.duration_seconds;
    target.animated = hit.animated;
    target.secure_resource = hit.secure_resource;
    target.can_download = hit.downloadable && !hit.protected_media && !hit.drm_protected;
    target.can_copy = hit.copyable;
    target.can_capture_frame = hit.capturable_frame && !hit.drm_protected;
    target.protected_media = hit.protected_media || hit.drm_protected;
    target.same_origin_with_page = hit.same_origin_with_page;
    return target;
  }

 private:
  [[nodiscard]] static MediaKind to_media_kind(EngineMediaElementKind kind) {
    switch (kind) {
      case EngineMediaElementKind::image: return MediaKind::image;
      case EngineMediaElementKind::video: return MediaKind::video;
      case EngineMediaElementKind::animated_image: return MediaKind::animated_image;
      case EngineMediaElementKind::thumbnail: return MediaKind::thumbnail;
      case EngineMediaElementKind::background_image: return MediaKind::background_image;
      case EngineMediaElementKind::embedded_media: return MediaKind::embedded_media;
      case EngineMediaElementKind::none: break;
    }
    return MediaKind::image;
  }
};

inline constexpr bool kMediaDetectorMayOverrideProtectedMedia = false;
static_assert(!kMediaDetectorMayOverrideProtectedMedia);

}  // namespace goreecloud::browser
