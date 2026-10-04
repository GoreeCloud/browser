#include <cassert>
#include <memory>

#include "goreecloud/browser/chromium_adapter.hpp"
#include "goreecloud/browser/native_engine_surface.hpp"

int main() {
  using namespace goreecloud::browser;

  ChromiumAdapterOptions adapter_options;
  adapter_options.runtime_root = ".";
  adapter_options.enable_sandbox = true;
  adapter_options.enable_site_isolation = true;

  ChromiumEngineAdapter engine(adapter_options);
  engine.initialize();

  EngineContextOptions context_options;
  context_options.profile_id = "chromium-smoke";
  context_options.storage_path = "chromium-smoke-profile";
  context_options.persistent_storage = true;

  auto context = engine.create_context(context_options);
  assert(context);

  EngineViewOptions view_options;
  view_options.initial_url = "https://example.com/";
  auto view = context->create_view(view_options);
  assert(view);
  assert(view->navigation_state().url == "https://example.com/");

  auto* attachable = dynamic_cast<NativeSurfaceAttachable*>(view.get());
  assert(attachable);
  auto* drop_forwarder =
      dynamic_cast<NativeSurfaceDropForwarder*>(view.get());
  assert(drop_forwarder);
  NativeEngineSurface surface;
  surface.window_handle = 1;
  surface.width = 1280;
  surface.height = 684;
  assert(attachable->attach_native_surface(surface));
  assert(attachable->native_surface_attached());
  attachable->detach_native_surface();
  assert(!attachable->native_surface_attached());

  assert(context->clear_all_data(kAllTemporaryEngineData));
  assert(context->clear_authentication_state(std::nullopt));
  assert(context->clear_permission_state(std::nullopt));

  engine.shutdown();
  return 0;
}
