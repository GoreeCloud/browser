#include <jni.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/file_normal_session_store.hpp"
#include "goreecloud/browser/normal_session_runtime.hpp"

namespace {

using goreecloud::browser::FileNormalSessionStore;
using goreecloud::browser::NormalSessionReplayResult;
using goreecloud::browser::NormalSessionRuntimeCoordinator;
using goreecloud::browser::NormalSessionRuntimeOptions;
using goreecloud::browser::NormalSessionRuntimeHealth;
using goreecloud::browser::RecoverableWindow;
using goreecloud::browser::kNormalPrivacyContextId;

constexpr std::size_t kAndroidMaxTabs = 32;

std::optional<std::string> from_jstring(JNIEnv* env, jstring value) {
  if (value == nullptr) return std::nullopt;
  const char* raw = env->GetStringUTFChars(value, nullptr);
  if (raw == nullptr) return std::nullopt;
  std::string result{raw};
  env->ReleaseStringUTFChars(value, raw);
  return result;
}

jstring to_jstring(JNIEnv* env, std::string_view value) {
  return env->NewStringUTF(std::string{value}.c_str());
}

class AndroidNormalSessionBridge {
 public:
  AndroidNormalSessionBridge(std::filesystem::path directory, std::string fresh_epoch)
      : store_(std::move(directory)), fresh_epoch_(std::move(fresh_epoch)) {
    initialize();
  }

  [[nodiscard]] bool available() const noexcept {
    return runtime_ != nullptr &&
           runtime_->status().health == NormalSessionRuntimeHealth::healthy;
  }

  [[nodiscard]] bool recovered() const noexcept {
    return recovered_.has_value();
  }

  [[nodiscard]] const std::optional<NormalSessionReplayResult>& recovered_state() const noexcept {
    return recovered_;
  }

  [[nodiscard]] bool discard_recovered_and_restart(std::string fresh_epoch) {
    if (!store_.erase_all()) return false;
    runtime_.reset();
    recovered_.reset();
    seeded_ = false;
    fresh_epoch_ = std::move(fresh_epoch);
    start_fresh();
    return available();
  }

  [[nodiscard]] bool seed(
      std::string_view window_id,
      const std::vector<std::string>& flat_tabs,
      std::string_view active_tab_id) {
    if (!available() || recovered() || seeded_ || flat_tabs.empty() ||
        flat_tabs.size() % 3 != 0 || flat_tabs.size() / 3 > kAndroidMaxTabs) {
      return false;
    }

    if (!runtime_->normal_window_opened(window_id)) return abandon_partial_seed();

    for (std::size_t index = 0; index < flat_tabs.size() / 3; ++index) {
      const auto offset = index * 3;
      if (!runtime_->normal_tab_opened(
              window_id,
              flat_tabs[offset],
              flat_tabs[offset + 1],
              flat_tabs[offset + 2],
              index)) {
        return abandon_partial_seed();
      }
    }

    if (!runtime_->normal_tab_selected(window_id, active_tab_id)) {
      return abandon_partial_seed();
    }
    seeded_ = true;
    return true;
  }

  [[nodiscard]] bool tab_opened(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view url,
      std::string_view title,
      std::size_t position) {
    return available() && seeded_ &&
           runtime_->normal_tab_opened(window_id, tab_id, url, title, position);
  }

  [[nodiscard]] bool tab_selected(
      std::string_view window_id,
      std::string_view tab_id) {
    return available() && seeded_ &&
           runtime_->normal_tab_selected(window_id, tab_id);
  }

  [[nodiscard]] bool tab_closed(
      std::string_view window_id,
      std::string_view tab_id) {
    return available() && seeded_ &&
           runtime_->normal_tab_closed(window_id, tab_id);
  }

  [[nodiscard]] bool tab_navigated(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view url) {
    return available() && seeded_ &&
           runtime_->normal_tab_navigated(window_id, tab_id, url);
  }

  [[nodiscard]] bool tab_titled(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view title) {
    return available() && seeded_ &&
           runtime_->normal_tab_titled(window_id, tab_id, title);
  }

  [[nodiscard]] bool background() {
    return available() && seeded_ && runtime_->background();
  }

  [[nodiscard]] bool resume() {
    return available() && seeded_ && runtime_->resume();
  }

  [[nodiscard]] bool clean_shutdown() {
    return runtime_ != nullptr && seeded_ && runtime_->clean_shutdown();
  }

 private:
  void initialize() {
    const auto replayed = store_.replay_from_disk();
    if (!replayed.accepted) {
      if (!store_.erase_all()) return;
      start_fresh();
      return;
    }

    if (representable_recovery(replayed)) {
      NormalSessionRuntimeOptions options{
          .journal_id = replayed.journal_id,
          .profile_id = replayed.profile_id,
          .session_epoch = replayed.session_epoch,
          .checkpoint_id_prefix = "android-checkpoint",
          .checkpoint_every_mutations = 8,
      };
      runtime_ = std::make_unique<NormalSessionRuntimeCoordinator>(
          store_, std::move(options));
      recovered_ = replayed;
      seeded_ = true;
      if (!runtime_->resume_recovery()) {
        recovered_.reset();
        seeded_ = false;
        runtime_.reset();
        if (!store_.erase_all()) return;
        start_fresh();
      }
      return;
    }

    if (replayed.inferred_abnormal_termination && !store_.erase_all()) return;
    start_fresh();
  }

  void start_fresh() {
    if (fresh_epoch_.empty()) return;
    NormalSessionRuntimeOptions options{
        .journal_id = "android-" + fresh_epoch_,
        .profile_id = "default",
        .session_epoch = fresh_epoch_,
        .checkpoint_id_prefix = "android-checkpoint",
        .checkpoint_every_mutations = 8,
    };
    runtime_ = std::make_unique<NormalSessionRuntimeCoordinator>(
        store_, std::move(options));
    if (!runtime_->begin()) runtime_.reset();
  }

  [[nodiscard]] static bool representable_recovery(
      const NormalSessionReplayResult& replayed) {
    if (!replayed.restore_eligible ||
        replayed.privacy_context_id != kNormalPrivacyContextId ||
        replayed.windows.size() != 1) {
      return false;
    }
    const RecoverableWindow& window = replayed.windows.front();
    if (window.tabs.empty() || window.tabs.size() > kAndroidMaxTabs ||
        !window.active_tab_id.has_value()) {
      return false;
    }
    for (const auto& tab : window.tabs) {
      if (tab.tab_id == *window.active_tab_id) return true;
    }
    return false;
  }

  [[nodiscard]] bool abandon_partial_seed() {
    (void)store_.erase_all();
    runtime_.reset();
    seeded_ = false;
    return false;
  }

  FileNormalSessionStore store_;
  std::string fresh_epoch_;
  std::unique_ptr<NormalSessionRuntimeCoordinator> runtime_;
  std::optional<NormalSessionReplayResult> recovered_;
  bool seeded_{false};
};

AndroidNormalSessionBridge* bridge_from(jlong handle) {
  return reinterpret_cast<AndroidNormalSessionBridge*>(
      static_cast<std::uintptr_t>(handle));
}

jlong handle_for(AndroidNormalSessionBridge* bridge) {
  return static_cast<jlong>(
      reinterpret_cast<std::uintptr_t>(bridge));
}

std::optional<std::vector<std::string>> flat_tabs_from_java(
    JNIEnv* env,
    jobjectArray array) {
  if (array == nullptr) return std::nullopt;
  const auto length = env->GetArrayLength(array);
  if (length <= 0 || length % 3 != 0 ||
      static_cast<std::size_t>(length / 3) > kAndroidMaxTabs) {
    return std::nullopt;
  }

  std::vector<std::string> result;
  result.reserve(static_cast<std::size_t>(length));
  for (jsize index = 0; index < length; ++index) {
    auto* value = static_cast<jstring>(env->GetObjectArrayElement(array, index));
    const auto converted = from_jstring(env, value);
    env->DeleteLocalRef(value);
    if (!converted.has_value()) return std::nullopt;
    result.push_back(*converted);
  }
  return result;
}

}  // namespace

extern "C" JNIEXPORT jlong JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeOpen(
    JNIEnv* env,
    jobject,
    jstring directory,
    jstring fresh_epoch) {
  const auto path = from_jstring(env, directory);
  const auto epoch = from_jstring(env, fresh_epoch);
  if (!path.has_value() || path->empty() || !epoch.has_value() || epoch->empty()) return 0;

  auto bridge = std::make_unique<AndroidNormalSessionBridge>(
      std::filesystem::path{*path}, *epoch);
  if (!bridge->available()) return 0;
  return handle_for(bridge.release());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeWasRecovered(
    JNIEnv*,
    jobject,
    jlong handle) {
  const auto* bridge = bridge_from(handle);
  return bridge != nullptr && bridge->recovered() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeRecoveredFlatTabs(
    JNIEnv* env,
    jobject,
    jlong handle) {
  const auto* bridge = bridge_from(handle);
  if (bridge == nullptr || !bridge->recovered_state().has_value()) return nullptr;
  const auto& windows = bridge->recovered_state()->windows;
  if (windows.size() != 1) return nullptr;

  const auto& tabs = windows.front().tabs;
  auto* string_class = env->FindClass("java/lang/String");
  if (string_class == nullptr) return nullptr;
  auto* result = env->NewObjectArray(
      static_cast<jsize>(tabs.size() * 3), string_class, nullptr);
  env->DeleteLocalRef(string_class);
  if (result == nullptr) return nullptr;

  for (std::size_t index = 0; index < tabs.size(); ++index) {
    const auto offset = static_cast<jsize>(index * 3);
    const auto& tab = tabs[index];
    auto* id = to_jstring(env, tab.tab_id);
    auto* url = to_jstring(env, tab.url);
    auto* title = to_jstring(env, tab.title);
    env->SetObjectArrayElement(result, offset, id);
    env->SetObjectArrayElement(result, offset + 1, url);
    env->SetObjectArrayElement(result, offset + 2, title);
    env->DeleteLocalRef(id);
    env->DeleteLocalRef(url);
    env->DeleteLocalRef(title);
  }
  return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeRecoveredActiveTabId(
    JNIEnv* env,
    jobject,
    jlong handle) {
  const auto* bridge = bridge_from(handle);
  if (bridge == nullptr || !bridge->recovered_state().has_value()) return nullptr;
  const auto& windows = bridge->recovered_state()->windows;
  if (windows.size() != 1 || !windows.front().active_tab_id.has_value()) return nullptr;
  return to_jstring(env, *windows.front().active_tab_id);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeDiscardRecoveredAndRestart(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring fresh_epoch) {
  auto* bridge = bridge_from(handle);
  const auto epoch = from_jstring(env, fresh_epoch);
  return bridge != nullptr && epoch.has_value() && !epoch->empty() &&
                 bridge->discard_recovered_and_restart(*epoch)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeSeed(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring window_id,
    jobjectArray flat_tabs,
    jstring active_tab_id) {
  auto* bridge = bridge_from(handle);
  const auto window = from_jstring(env, window_id);
  const auto active = from_jstring(env, active_tab_id);
  const auto tabs = flat_tabs_from_java(env, flat_tabs);
  return bridge != nullptr && window.has_value() && active.has_value() &&
                 tabs.has_value() && bridge->seed(*window, *tabs, *active)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeTabOpened(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring window_id,
    jstring tab_id,
    jstring url,
    jstring title,
    jint position) {
  auto* bridge = bridge_from(handle);
  const auto window = from_jstring(env, window_id);
  const auto tab = from_jstring(env, tab_id);
  const auto location = from_jstring(env, url);
  const auto label = from_jstring(env, title);
  return bridge != nullptr && window.has_value() && tab.has_value() &&
                 location.has_value() && label.has_value() && position >= 0 &&
                 bridge->tab_opened(
                     *window, *tab, *location, *label,
                     static_cast<std::size_t>(position))
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeTabSelected(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring window_id,
    jstring tab_id) {
  auto* bridge = bridge_from(handle);
  const auto window = from_jstring(env, window_id);
  const auto tab = from_jstring(env, tab_id);
  return bridge != nullptr && window.has_value() && tab.has_value() &&
                 bridge->tab_selected(*window, *tab)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeTabClosed(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring window_id,
    jstring tab_id) {
  auto* bridge = bridge_from(handle);
  const auto window = from_jstring(env, window_id);
  const auto tab = from_jstring(env, tab_id);
  return bridge != nullptr && window.has_value() && tab.has_value() &&
                 bridge->tab_closed(*window, *tab)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeTabNavigated(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring window_id,
    jstring tab_id,
    jstring url) {
  auto* bridge = bridge_from(handle);
  const auto window = from_jstring(env, window_id);
  const auto tab = from_jstring(env, tab_id);
  const auto location = from_jstring(env, url);
  return bridge != nullptr && window.has_value() && tab.has_value() &&
                 location.has_value() &&
                 bridge->tab_navigated(*window, *tab, *location)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeTabTitled(
    JNIEnv* env,
    jobject,
    jlong handle,
    jstring window_id,
    jstring tab_id,
    jstring title) {
  auto* bridge = bridge_from(handle);
  const auto window = from_jstring(env, window_id);
  const auto tab = from_jstring(env, tab_id);
  const auto label = from_jstring(env, title);
  return bridge != nullptr && window.has_value() && tab.has_value() &&
                 label.has_value() &&
                 bridge->tab_titled(*window, *tab, *label)
             ? JNI_TRUE
             : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeBackground(
    JNIEnv*,
    jobject,
    jlong handle) {
  auto* bridge = bridge_from(handle);
  return bridge != nullptr && bridge->background() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeResume(
    JNIEnv*,
    jobject,
    jlong handle) {
  auto* bridge = bridge_from(handle);
  return bridge != nullptr && bridge->resume() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeCleanShutdown(
    JNIEnv*,
    jobject,
    jlong handle) {
  auto* bridge = bridge_from(handle);
  return bridge != nullptr && bridge->clean_shutdown() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_io_goreecloud_browser_BrowserNormalSessionNative_nativeClose(
    JNIEnv*,
    jobject,
    jlong handle) {
  delete bridge_from(handle);
}
