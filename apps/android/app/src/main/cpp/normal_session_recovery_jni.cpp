#include <jni.h>

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/file_normal_session_store.hpp"
#include "goreecloud/browser/normal_session_runtime.hpp"

namespace {

using goreecloud::browser::FileNormalSessionStore;
using goreecloud::browser::NormalSessionLifecycleState;
using goreecloud::browser::NormalSessionReplayResult;
using goreecloud::browser::NormalSessionRuntimeCoordinator;
using goreecloud::browser::NormalSessionRuntimeOptions;
using goreecloud::browser::NormalSessionStartupClassification;
using goreecloud::browser::SessionPrivacyMode;

constexpr std::string_view kAndroidWindowId = "android-main";
constexpr std::size_t kAndroidMaxTabs = 32;

struct AndroidRuntimeState {
  std::unique_ptr<FileNormalSessionStore> store;
  std::unique_ptr<NormalSessionRuntimeCoordinator> runtime;
};

std::mutex g_mutex;
std::unique_ptr<AndroidRuntimeState> g_state;

std::optional<std::string> java_string(JNIEnv* env, jstring value) {
  if (value == nullptr) return std::nullopt;
  const char* chars = env->GetStringUTFChars(value, nullptr);
  if (chars == nullptr) return std::nullopt;
  std::string result{chars};
  env->ReleaseStringUTFChars(value, chars);
  return result;
}

jobjectArray string_array(JNIEnv* env, const std::vector<std::string>& values) {
  const auto string_class = env->FindClass("java/lang/String");
  if (string_class == nullptr) return nullptr;

  auto array = env->NewObjectArray(
      static_cast<jsize>(values.size()),
      string_class,
      nullptr);
  if (array == nullptr) return nullptr;

  for (std::size_t index = 0; index < values.size(); ++index) {
    auto value = env->NewStringUTF(values[index].c_str());
    if (value == nullptr) return nullptr;
    env->SetObjectArrayElement(array, static_cast<jsize>(index), value);
    env->DeleteLocalRef(value);
  }
  return array;
}

jobjectArray marker(JNIEnv* env, std::string value) {
  return string_array(env, std::vector<std::string>{std::move(value)});
}

bool has_no_durable_state(const NormalSessionReplayResult& replayed) {
  return replayed.journal_high_water_mark == 0 &&
         replayed.journal_id.empty() &&
         replayed.profile_id.empty() &&
         replayed.session_epoch.empty() &&
         replayed.windows.empty() &&
         replayed.retired_window_ids.empty() &&
         replayed.retired_tab_ids.empty();
}

bool valid_android_projection(const NormalSessionReplayResult& replayed) {
  if (!replayed.accepted ||
      replayed.privacy_context_id != goreecloud::browser::kNormalPrivacyContextId ||
      replayed.windows.size() > 1) {
    return false;
  }
  if (replayed.windows.empty()) return true;

  const auto& window = replayed.windows.front();
  if (window.window_id != kAndroidWindowId ||
      window.privacy_mode != SessionPrivacyMode::normal ||
      window.tabs.size() > kAndroidMaxTabs) {
    return false;
  }
  if (window.tabs.empty()) {
    return !window.active_tab_id.has_value();
  }
  return window.active_tab_id.has_value();
}

std::vector<std::string> recovered_projection(
    const NormalSessionReplayResult& replayed) {
  if (replayed.windows.empty() || replayed.windows.front().tabs.empty()) {
    return {"recovered_empty"};
  }

  const auto& window = replayed.windows.front();
  std::vector<std::string> result;
  result.reserve(2 + window.tabs.size() * 3);
  result.push_back("recovered");
  result.push_back(window.active_tab_id.value_or(""));
  for (const auto& tab : window.tabs) {
    result.push_back(tab.tab_id);
    result.push_back(tab.url);
    result.push_back(tab.title);
  }
  return result;
}

template <typename Callback>
jboolean with_runtime(Callback&& callback) {
  std::lock_guard lock{g_mutex};
  if (!g_state || !g_state->runtime) return JNI_FALSE;
  return callback(*g_state->runtime) ? JNI_TRUE : JNI_FALSE;
}

}  // namespace

extern "C" JNIEXPORT jobjectArray JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_start(
    JNIEnv* env,
    jobject,
    jstring storage_directory,
    jstring journal_id,
    jstring profile_id,
    jstring session_epoch,
    jstring checkpoint_prefix) {
  const auto directory = java_string(env, storage_directory);
  const auto requested_journal = java_string(env, journal_id);
  const auto requested_profile = java_string(env, profile_id);
  const auto requested_epoch = java_string(env, session_epoch);
  const auto requested_checkpoint_prefix = java_string(env, checkpoint_prefix);
  if (!directory || !requested_journal || !requested_profile ||
      !requested_epoch || !requested_checkpoint_prefix) {
    return marker(env, "unavailable");
  }

  std::lock_guard lock{g_mutex};
  if (g_state && g_state->runtime) {
    return marker(
        env,
        g_state->runtime->accepting_runtime_events() ? "reused" : "unavailable");
  }

  auto store = std::make_unique<FileNormalSessionStore>(*directory);
  const auto replayed = store->replay_from_disk();
  if (!replayed.accepted) {
    return marker(env, "unavailable");
  }

  NormalSessionRuntimeOptions options{
      .journal_id = *requested_journal,
      .profile_id = *requested_profile,
      .session_epoch = *requested_epoch,
      .checkpoint_id_prefix = *requested_checkpoint_prefix,
      .checkpoint_every_mutations = 16,
  };
  auto runtime =
      std::make_unique<NormalSessionRuntimeCoordinator>(*store, std::move(options));

  const bool no_state = has_no_durable_state(replayed);
  const bool clean =
      !no_state &&
      replayed.lifecycle_state == NormalSessionLifecycleState::clean_shutdown;

  if (no_state || clean) {
    if (!runtime->begin()) {
      return marker(env, "unavailable");
    }
    g_state = std::make_unique<AndroidRuntimeState>(
        AndroidRuntimeState{std::move(store), std::move(runtime)});
    return marker(env, clean ? "clean_fresh" : "fresh");
  }

  if (!valid_android_projection(replayed) ||
      !runtime->resume_recovered_session()) {
    return marker(env, "unavailable");
  }

  const auto projection = recovered_projection(replayed);
  g_state = std::make_unique<AndroidRuntimeState>(
      AndroidRuntimeState{std::move(store), std::move(runtime)});
  return string_array(env, projection);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_windowOpened(
    JNIEnv* env,
    jobject,
    jstring window_id) {
  const auto id = java_string(env, window_id);
  if (!id) return JNI_FALSE;
  return with_runtime(
      [&](auto& runtime) { return runtime.normal_window_opened(*id); });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_windowClosed(
    JNIEnv* env,
    jobject,
    jstring window_id) {
  const auto id = java_string(env, window_id);
  if (!id) return JNI_FALSE;
  return with_runtime(
      [&](auto& runtime) { return runtime.normal_window_closed(*id); });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_tabOpened(
    JNIEnv* env,
    jobject,
    jstring window_id,
    jstring tab_id,
    jstring url,
    jstring title,
    jint position) {
  const auto window = java_string(env, window_id);
  const auto tab = java_string(env, tab_id);
  const auto location = java_string(env, url);
  const auto label = java_string(env, title);
  if (!window || !tab || !location || !label || position < 0) return JNI_FALSE;
  return with_runtime([&](auto& runtime) {
    return runtime.normal_tab_opened(
        *window,
        *tab,
        *location,
        *label,
        static_cast<std::size_t>(position));
  });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_tabSelected(
    JNIEnv* env,
    jobject,
    jstring window_id,
    jstring tab_id) {
  const auto window = java_string(env, window_id);
  const auto tab = java_string(env, tab_id);
  if (!window || !tab) return JNI_FALSE;
  return with_runtime(
      [&](auto& runtime) { return runtime.normal_tab_selected(*window, *tab); });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_tabClosed(
    JNIEnv* env,
    jobject,
    jstring window_id,
    jstring tab_id) {
  const auto window = java_string(env, window_id);
  const auto tab = java_string(env, tab_id);
  if (!window || !tab) return JNI_FALSE;
  return with_runtime(
      [&](auto& runtime) { return runtime.normal_tab_closed(*window, *tab); });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_tabNavigated(
    JNIEnv* env,
    jobject,
    jstring window_id,
    jstring tab_id,
    jstring url) {
  const auto window = java_string(env, window_id);
  const auto tab = java_string(env, tab_id);
  const auto location = java_string(env, url);
  if (!window || !tab || !location) return JNI_FALSE;
  return with_runtime([&](auto& runtime) {
    return runtime.normal_tab_navigated(*window, *tab, *location);
  });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_tabTitled(
    JNIEnv* env,
    jobject,
    jstring window_id,
    jstring tab_id,
    jstring title) {
  const auto window = java_string(env, window_id);
  const auto tab = java_string(env, tab_id);
  const auto label = java_string(env, title);
  if (!window || !tab || !label) return JNI_FALSE;
  return with_runtime([&](auto& runtime) {
    return runtime.normal_tab_titled(*window, *tab, *label);
  });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_background(
    JNIEnv*,
    jobject) {
  return with_runtime([](auto& runtime) { return runtime.background(); });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_resume(
    JNIEnv*,
    jobject) {
  return with_runtime([](auto& runtime) { return runtime.resume(); });
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_cleanShutdown(
    JNIEnv*,
    jobject) {
  return with_runtime([](auto& runtime) { return runtime.clean_shutdown(); });
}

extern "C" JNIEXPORT void JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_release(
    JNIEnv*,
    jobject) {
  std::lock_guard lock{g_mutex};
  g_state.reset();
}
