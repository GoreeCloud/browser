#pragma once

#include <cstdint>

namespace goreecloud::browser {

struct NativeSurfaceFrame {
  const std::uint8_t* bgra{nullptr};
  int width{0};
  int height{0};
  int stride{0};
  float scale_factor{1.0F};
};

// Receives engine-owned software frames synchronously. Implementations must copy
// any pixels they need to retain after present_software_frame returns.
class NativeSurfaceFrameSink {
 public:
  virtual ~NativeSurfaceFrameSink() = default;
  virtual void present_software_frame(const NativeSurfaceFrame& frame) = 0;
};

// Opaque platform surface description used to attach an engine-rendered view
// to GoreeCloud-owned native chrome without exposing Chromium types upstream.
//
// window_handle/display_handle describe the native child-window path. frame_sink
// describes the windowless/software path used when the host cannot provide a
// child window (for example a GTK Wayland backend).
struct NativeEngineSurface {
  std::uintptr_t window_handle{0};
  std::uintptr_t display_handle{0};
  int x{0};
  int y{0};
  int width{0};
  int height{0};
  float scale_factor{1.0F};
  NativeSurfaceFrameSink* frame_sink{nullptr};
};

class NativeSurfaceAttachable {
 public:
  virtual ~NativeSurfaceAttachable() = default;
  virtual bool attach_native_surface(const NativeEngineSurface& surface) = 0;
  virtual void resize_native_surface(const NativeEngineSurface& surface) = 0;
  virtual void detach_native_surface() = 0;
  [[nodiscard]] virtual bool native_surface_attached() const noexcept = 0;
};

}  // namespace goreecloud::browser
