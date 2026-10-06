#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "goreecloud/browser/session_recovery.hpp"

namespace goreecloud::browser {

inline constexpr std::uint64_t kNormalSessionJournalSchemaVersion = 1;
inline constexpr std::size_t kMaxNormalSessionJournalEntries = 16'384;
inline constexpr std::size_t kMaxNormalSessionRetiredIds = 8'192;
inline constexpr std::size_t kMaxNormalSessionUrlBytes = 8'192;
inline constexpr std::size_t kMaxNormalSessionTitleBytes = 512;
inline constexpr std::string_view kNormalPrivacyContextId = "normal";

enum class NormalSessionLifecycleState {
  starting,
  running,
  backgrounding,
  checkpointing,
  restoring,
  clean_shutdown,
  abnormal_termination,
};

enum class NormalSessionJournalOperation {
  lifecycle,
  open_window,
  close_window,
  open_tab,
  select_tab,
  close_tab,
  navigate_tab,
  title_tab,
  reorder_tab,
};

struct NormalSessionJournalEntry {
  std::uint64_t schema_version{kNormalSessionJournalSchemaVersion};
  std::string journal_id;
  std::string profile_id;
  std::string privacy_context_id{std::string{kNormalPrivacyContextId}};
  std::string session_epoch;
  std::uint64_t sequence{0};
  std::uint64_t created_unix_ms{0};
  NormalSessionJournalOperation operation{NormalSessionJournalOperation::lifecycle};
  NormalSessionLifecycleState lifecycle_state{NormalSessionLifecycleState::starting};
  std::string window_id;
  std::string tab_id;
  std::optional<std::size_t> position;
  std::string url;
  std::string title;

  bool operator==(const NormalSessionJournalEntry&) const = default;
};

struct NormalSessionCheckpoint {
  std::uint64_t schema_version{kNormalSessionJournalSchemaVersion};
  std::string checkpoint_id;
  std::string journal_id;
  std::string profile_id;
  std::string privacy_context_id{std::string{kNormalPrivacyContextId}};
  std::string session_epoch;
  std::uint64_t created_unix_ms{0};
  std::uint64_t journal_high_water_mark{0};
  NormalSessionLifecycleState lifecycle_state{NormalSessionLifecycleState::starting};
  std::vector<RecoverableWindow> windows;
  std::vector<std::string> retired_window_ids;
  std::vector<std::string> retired_tab_ids;
  std::string integrity_checksum;
};

struct NormalSessionReplayResult {
  bool accepted{false};
  bool restore_eligible{false};
  bool inferred_abnormal_termination{false};
  std::string journal_id;
  std::string profile_id;
  std::string privacy_context_id;
  std::string session_epoch;
  std::uint64_t journal_high_water_mark{0};
  NormalSessionLifecycleState lifecycle_state{NormalSessionLifecycleState::starting};
  std::vector<RecoverableWindow> windows;
  std::vector<std::string> retired_window_ids;
  std::vector<std::string> retired_tab_ids;
  std::string error;
};

class NormalSessionDurableStore {
 public:
  virtual ~NormalSessionDurableStore() = default;

  [[nodiscard]] virtual bool append(const NormalSessionJournalEntry& entry) = 0;
  [[nodiscard]] virtual bool compact(const NormalSessionCheckpoint& checkpoint) = 0;
  [[nodiscard]] virtual NormalSessionReplayResult replay_from_disk() const = 0;
};

class NormalSessionJournalPolicy {
 public:
  [[nodiscard]] static NormalSessionReplayResult replay(
      std::optional<NormalSessionCheckpoint> checkpoint,
      std::vector<NormalSessionJournalEntry> entries) {
    NormalSessionReplayResult result;

    if (entries.size() > kMaxNormalSessionJournalEntries) {
      return reject("journal entry bound exceeded");
    }

    if (checkpoint.has_value()) {
      if (!valid_checkpoint(*checkpoint)) {
        return reject("invalid checkpoint");
      }
      result.journal_id = checkpoint->journal_id;
      result.profile_id = checkpoint->profile_id;
      result.privacy_context_id = checkpoint->privacy_context_id;
      result.session_epoch = checkpoint->session_epoch;
      result.journal_high_water_mark = checkpoint->journal_high_water_mark;
      result.lifecycle_state = checkpoint->lifecycle_state;
      result.windows = checkpoint->windows;
      result.retired_window_ids = checkpoint->retired_window_ids;
      result.retired_tab_ids = checkpoint->retired_tab_ids;
    }

    std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right) {
      return left.sequence < right.sequence;
    });

    std::vector<NormalSessionJournalEntry> deduplicated;
    deduplicated.reserve(entries.size());
    for (const auto& entry : entries) {
      if (!valid_entry(entry)) {
        return reject("invalid journal entry");
      }

      if (!deduplicated.empty() && deduplicated.back().sequence == entry.sequence) {
        if (!(deduplicated.back() == entry)) {
          return reject("conflicting journal sequence");
        }
        continue;
      }
      deduplicated.push_back(entry);
    }

    if (!checkpoint.has_value() && !deduplicated.empty()) {
      if (deduplicated.front().sequence != 1) {
        return reject("journal does not begin at sequence one");
      }
      result.journal_id = deduplicated.front().journal_id;
      result.profile_id = deduplicated.front().profile_id;
      result.privacy_context_id = deduplicated.front().privacy_context_id;
      result.session_epoch = deduplicated.front().session_epoch;
    }

    std::uint64_t expected_sequence =
        checkpoint.has_value() ? checkpoint->journal_high_water_mark + 1 : 1;
    for (const auto& entry : deduplicated) {
      if (entry.sequence <= result.journal_high_water_mark) continue;
      if (entry.sequence != expected_sequence) {
        return reject("journal sequence gap");
      }
      ++expected_sequence;
    }

    if (!valid_identity(result.journal_id) && !deduplicated.empty()) {
      return reject("missing journal identity");
    }

    std::unordered_set<std::string> retired_windows(result.retired_window_ids.begin(),
                                                     result.retired_window_ids.end());
    std::unordered_set<std::string> retired_tabs(result.retired_tab_ids.begin(),
                                                  result.retired_tab_ids.end());

    for (const auto& entry : deduplicated) {
      if (!same_identity(result, entry)) {
        return reject("journal identity mismatch");
      }

      if (entry.sequence <= result.journal_high_water_mark) {
        continue;
      }

      if (!apply_entry(result, retired_windows, retired_tabs, entry)) {
        return reject("journal transition rejected");
      }
      result.journal_high_water_mark = entry.sequence;
    }

    result.retired_window_ids.assign(retired_windows.begin(), retired_windows.end());
    result.retired_tab_ids.assign(retired_tabs.begin(), retired_tabs.end());
    std::sort(result.retired_window_ids.begin(), result.retired_window_ids.end());
    std::sort(result.retired_tab_ids.begin(), result.retired_tab_ids.end());

    if (!valid_replayed_state(result)) {
      return reject("replayed state is inconsistent");
    }

    result.accepted = true;
    result.inferred_abnormal_termination =
        result.lifecycle_state != NormalSessionLifecycleState::clean_shutdown;
    result.restore_eligible =
        result.inferred_abnormal_termination && !result.windows.empty();
    return result;
  }

  [[nodiscard]] static std::optional<NormalSessionCheckpoint> checkpoint_from_replay(
      const NormalSessionReplayResult& replayed,
      std::string checkpoint_id,
      std::uint64_t created_unix_ms) {
    if (!replayed.accepted || !valid_identity(checkpoint_id) || created_unix_ms == 0) {
      return std::nullopt;
    }

    NormalSessionCheckpoint checkpoint;
    checkpoint.checkpoint_id = std::move(checkpoint_id);
    checkpoint.journal_id = replayed.journal_id;
    checkpoint.profile_id = replayed.profile_id;
    checkpoint.privacy_context_id = replayed.privacy_context_id;
    checkpoint.session_epoch = replayed.session_epoch;
    checkpoint.created_unix_ms = created_unix_ms;
    checkpoint.journal_high_water_mark = replayed.journal_high_water_mark;
    checkpoint.lifecycle_state = replayed.lifecycle_state;
    checkpoint.windows = replayed.windows;
    checkpoint.retired_window_ids = replayed.retired_window_ids;
    checkpoint.retired_tab_ids = replayed.retired_tab_ids;
    return seal_checkpoint(std::move(checkpoint));
  }

  [[nodiscard]] static std::optional<NormalSessionCheckpoint> seal_checkpoint(
      NormalSessionCheckpoint checkpoint) {
    checkpoint.integrity_checksum.clear();
    std::sort(checkpoint.retired_window_ids.begin(), checkpoint.retired_window_ids.end());
    std::sort(checkpoint.retired_tab_ids.begin(), checkpoint.retired_tab_ids.end());
    if (!valid_checkpoint_structure(checkpoint)) {
      return std::nullopt;
    }
    checkpoint.integrity_checksum = checksum_for(checkpoint);
    return checkpoint;
  }

  [[nodiscard]] static bool valid_checkpoint(const NormalSessionCheckpoint& checkpoint) {
    if (!valid_checkpoint_structure(checkpoint) || checkpoint.integrity_checksum.empty()) {
      return false;
    }
    auto copy = checkpoint;
    const auto supplied = copy.integrity_checksum;
    copy.integrity_checksum.clear();
    return supplied == checksum_for(copy);
  }

  [[nodiscard]] static bool valid_journal_entry(
      const NormalSessionJournalEntry& entry) {
    return valid_entry(entry);
  }

  [[nodiscard]] static bool safe_restoration_url(std::string_view url) {
    if (url.empty()) return true;
    if (url.size() > kMaxNormalSessionUrlBytes) return false;

    for (const unsigned char ch : url) {
      if (std::iscntrl(ch) || std::isspace(ch)) return false;
    }

    const bool http = url.starts_with("http://");
    const bool https = url.starts_with("https://");
    if (!http && !https) return false;

    const auto authority_start = http ? 7U : 8U;
    if (url.size() <= authority_start) return false;
    const auto authority_end = url.find_first_of("/?#", authority_start);
    const auto authority = url.substr(
        authority_start,
        authority_end == std::string_view::npos ? url.size() - authority_start
                                                : authority_end - authority_start);
    if (authority.empty() || authority.find('@') != std::string_view::npos) return false;
    return true;
  }

 private:
  [[nodiscard]] static NormalSessionReplayResult reject(std::string error) {
    NormalSessionReplayResult result;
    result.error = std::move(error);
    return result;
  }

  [[nodiscard]] static bool valid_identity(std::string_view value) {
    return !value.empty() && value.size() <= kMaxRecoveryIdentifierBytes;
  }

  [[nodiscard]] static bool valid_title(std::string_view title) {
    return title.size() <= kMaxNormalSessionTitleBytes;
  }

  [[nodiscard]] static bool valid_entry(const NormalSessionJournalEntry& entry) {
    if (entry.schema_version != kNormalSessionJournalSchemaVersion ||
        !valid_identity(entry.journal_id) || !valid_identity(entry.profile_id) ||
        entry.privacy_context_id != kNormalPrivacyContextId ||
        !valid_identity(entry.session_epoch) || entry.sequence == 0 ||
        entry.created_unix_ms == 0 || !valid_title(entry.title)) {
      return false;
    }

    const auto valid_window = valid_identity(entry.window_id);
    const auto valid_tab = valid_identity(entry.tab_id);
    const auto position_in_bounds =
        !entry.position.has_value() || *entry.position <= kMaxRecoveryTabsPerWindow;

    switch (entry.operation) {
      case NormalSessionJournalOperation::lifecycle:
        return entry.window_id.empty() && entry.tab_id.empty() &&
               !entry.position.has_value() && entry.url.empty() && entry.title.empty();
      case NormalSessionJournalOperation::open_window:
      case NormalSessionJournalOperation::close_window:
        return valid_window && entry.tab_id.empty() && !entry.position.has_value() &&
               entry.url.empty() && entry.title.empty();
      case NormalSessionJournalOperation::open_tab:
        return valid_window && valid_tab && position_in_bounds &&
               safe_restoration_url(entry.url);
      case NormalSessionJournalOperation::select_tab:
      case NormalSessionJournalOperation::close_tab:
        return valid_window && valid_tab && !entry.position.has_value() &&
               entry.url.empty() && entry.title.empty();
      case NormalSessionJournalOperation::navigate_tab:
        return valid_window && valid_tab && !entry.position.has_value() &&
               safe_restoration_url(entry.url) && entry.title.empty();
      case NormalSessionJournalOperation::title_tab:
        return valid_window && valid_tab && !entry.position.has_value() &&
               entry.url.empty();
      case NormalSessionJournalOperation::reorder_tab:
        return valid_window && valid_tab && entry.position.has_value() &&
               position_in_bounds && entry.url.empty() && entry.title.empty();
    }
    return false;
  }

  [[nodiscard]] static bool same_identity(
      const NormalSessionReplayResult& result,
      const NormalSessionJournalEntry& entry) {
    return result.journal_id == entry.journal_id &&
           result.profile_id == entry.profile_id &&
           result.privacy_context_id == entry.privacy_context_id &&
           result.session_epoch == entry.session_epoch;
  }

  [[nodiscard]] static RecoverableWindow* find_window(
      std::vector<RecoverableWindow>& windows,
      const std::string& window_id) {
    const auto found = std::find_if(
        windows.begin(), windows.end(),
        [&](const auto& window) { return window.window_id == window_id; });
    return found == windows.end() ? nullptr : &*found;
  }

  [[nodiscard]] static const RecoverableWindow* find_window(
      const std::vector<RecoverableWindow>& windows,
      const std::string& window_id) {
    const auto found = std::find_if(
        windows.begin(), windows.end(),
        [&](const auto& window) { return window.window_id == window_id; });
    return found == windows.end() ? nullptr : &*found;
  }

  [[nodiscard]] static RecoverableTab* find_tab(
      RecoverableWindow& window,
      const std::string& tab_id) {
    const auto found = std::find_if(
        window.tabs.begin(), window.tabs.end(),
        [&](const auto& tab) { return tab.tab_id == tab_id; });
    return found == window.tabs.end() ? nullptr : &*found;
  }

  [[nodiscard]] static const RecoverableTab* find_tab(
      const RecoverableWindow& window,
      const std::string& tab_id) {
    const auto found = std::find_if(
        window.tabs.begin(), window.tabs.end(),
        [&](const auto& tab) { return tab.tab_id == tab_id; });
    return found == window.tabs.end() ? nullptr : &*found;
  }

  [[nodiscard]] static bool tab_exists_elsewhere(
      const std::vector<RecoverableWindow>& windows,
      const std::string& window_id,
      const std::string& tab_id) {
    for (const auto& window : windows) {
      if (window.window_id != window_id && find_tab(window, tab_id) != nullptr) return true;
    }
    return false;
  }

  static void set_active_tab(RecoverableWindow& window, const std::string& tab_id) {
    window.active_tab_id = tab_id;
    for (auto& tab : window.tabs) {
      tab.active = tab.tab_id == tab_id;
    }
  }

  static void clear_active_tab(RecoverableWindow& window) {
    window.active_tab_id.reset();
    for (auto& tab : window.tabs) tab.active = false;
  }

  [[nodiscard]] static bool valid_lifecycle_transition(
      NormalSessionLifecycleState from,
      NormalSessionLifecycleState to) {
    if (from == NormalSessionLifecycleState::clean_shutdown ||
        from == NormalSessionLifecycleState::abnormal_termination) {
      return from == to;
    }
    return true;
  }

  [[nodiscard]] static bool apply_entry(
      NormalSessionReplayResult& result,
      std::unordered_set<std::string>& retired_windows,
      std::unordered_set<std::string>& retired_tabs,
      const NormalSessionJournalEntry& entry) {
    if (entry.operation != NormalSessionJournalOperation::lifecycle &&
        (result.lifecycle_state == NormalSessionLifecycleState::clean_shutdown ||
         result.lifecycle_state == NormalSessionLifecycleState::abnormal_termination)) {
      return false;
    }

    switch (entry.operation) {
      case NormalSessionJournalOperation::lifecycle:
        if (!valid_lifecycle_transition(result.lifecycle_state, entry.lifecycle_state)) {
          return false;
        }
        result.lifecycle_state = entry.lifecycle_state;
        return true;

      case NormalSessionJournalOperation::open_window: {
        if (retired_windows.contains(entry.window_id)) return false;
        if (find_window(result.windows, entry.window_id) != nullptr) return true;
        if (result.windows.size() >= kMaxRecoveryWindows) return false;
        RecoverableWindow window;
        window.window_id = entry.window_id;
        window.privacy_mode = SessionPrivacyMode::normal;
        result.windows.push_back(std::move(window));
        return true;
      }

      case NormalSessionJournalOperation::close_window: {
        const auto found = std::find_if(
            result.windows.begin(), result.windows.end(),
            [&](const auto& window) { return window.window_id == entry.window_id; });
        if (found != result.windows.end()) {
          for (const auto& tab : found->tabs) {
            retired_tabs.insert(tab.tab_id);
          }
          result.windows.erase(found);
        }
        retired_windows.insert(entry.window_id);
        return retired_windows.size() <= kMaxNormalSessionRetiredIds &&
               retired_tabs.size() <= kMaxNormalSessionRetiredIds;
      }

      case NormalSessionJournalOperation::open_tab: {
        if (retired_windows.contains(entry.window_id) || retired_tabs.contains(entry.tab_id) ||
            tab_exists_elsewhere(result.windows, entry.window_id, entry.tab_id)) {
          return false;
        }
        auto* window = find_window(result.windows, entry.window_id);
        if (window == nullptr || window->privacy_mode != SessionPrivacyMode::normal) return false;

        if (auto* existing = find_tab(*window, entry.tab_id); existing != nullptr) {
          return existing->url == entry.url && existing->title == entry.title;
        }
        if (window->tabs.size() >= kMaxRecoveryTabsPerWindow) return false;

        RecoverableTab tab;
        tab.tab_id = entry.tab_id;
        tab.url = entry.url;
        tab.title = entry.title;
        const auto position =
            std::min(entry.position.value_or(window->tabs.size()), window->tabs.size());
        window->tabs.insert(window->tabs.begin() + static_cast<std::ptrdiff_t>(position),
                            std::move(tab));
        if (!window->active_tab_id.has_value()) {
          set_active_tab(*window, entry.tab_id);
        }
        return true;
      }

      case NormalSessionJournalOperation::select_tab: {
        if (retired_tabs.contains(entry.tab_id)) return false;
        auto* window = find_window(result.windows, entry.window_id);
        if (window == nullptr || find_tab(*window, entry.tab_id) == nullptr) return false;
        set_active_tab(*window, entry.tab_id);
        return true;
      }

      case NormalSessionJournalOperation::close_tab: {
        auto* window = find_window(result.windows, entry.window_id);
        if (window == nullptr) {
          retired_tabs.insert(entry.tab_id);
          return retired_tabs.size() <= kMaxNormalSessionRetiredIds;
        }
        const auto found = std::find_if(
            window->tabs.begin(), window->tabs.end(),
            [&](const auto& tab) { return tab.tab_id == entry.tab_id; });
        if (found != window->tabs.end()) {
          const auto index = static_cast<std::size_t>(
              std::distance(window->tabs.begin(), found));
          const bool was_active = window->active_tab_id == entry.tab_id;
          window->tabs.erase(found);
          if (was_active) {
            if (window->tabs.empty()) {
              clear_active_tab(*window);
            } else {
              const auto next_index = std::min(index, window->tabs.size() - 1);
              set_active_tab(*window, window->tabs[next_index].tab_id);
            }
          }
        }
        retired_tabs.insert(entry.tab_id);
        return retired_tabs.size() <= kMaxNormalSessionRetiredIds;
      }

      case NormalSessionJournalOperation::navigate_tab: {
        if (retired_tabs.contains(entry.tab_id)) return true;
        auto* window = find_window(result.windows, entry.window_id);
        if (window == nullptr) return false;
        auto* tab = find_tab(*window, entry.tab_id);
        if (tab == nullptr) return false;
        tab->url = entry.url;
        return true;
      }

      case NormalSessionJournalOperation::title_tab: {
        if (retired_tabs.contains(entry.tab_id)) return true;
        auto* window = find_window(result.windows, entry.window_id);
        if (window == nullptr) return false;
        auto* tab = find_tab(*window, entry.tab_id);
        if (tab == nullptr) return false;
        tab->title = entry.title;
        return true;
      }

      case NormalSessionJournalOperation::reorder_tab: {
        if (retired_tabs.contains(entry.tab_id)) return false;
        auto* window = find_window(result.windows, entry.window_id);
        if (window == nullptr || !entry.position.has_value()) return false;
        const auto found = std::find_if(
            window->tabs.begin(), window->tabs.end(),
            [&](const auto& tab) { return tab.tab_id == entry.tab_id; });
        if (found == window->tabs.end()) return false;

        auto tab = std::move(*found);
        window->tabs.erase(found);
        const auto destination = std::min(*entry.position, window->tabs.size());
        window->tabs.insert(window->tabs.begin() + static_cast<std::ptrdiff_t>(destination),
                            std::move(tab));
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] static bool valid_replayed_state(const NormalSessionReplayResult& result) {
    if (!result.windows.empty() &&
        (!valid_identity(result.journal_id) || !valid_identity(result.profile_id) ||
         result.privacy_context_id != kNormalPrivacyContextId ||
         !valid_identity(result.session_epoch))) {
      return false;
    }

    if (result.windows.size() > kMaxRecoveryWindows ||
        result.retired_window_ids.size() > kMaxNormalSessionRetiredIds ||
        result.retired_tab_ids.size() > kMaxNormalSessionRetiredIds) {
      return false;
    }

    std::unordered_set<std::string> window_ids;
    std::unordered_set<std::string> tab_ids;
    std::size_t total_tabs = 0;
    for (const auto& window : result.windows) {
      if (window.privacy_mode != SessionPrivacyMode::normal ||
          !valid_identity(window.window_id) ||
          !window_ids.insert(window.window_id).second ||
          window.tabs.size() > kMaxRecoveryTabsPerWindow) {
        return false;
      }
      total_tabs += window.tabs.size();
      if (total_tabs > kMaxRecoveryTotalTabs) return false;

      std::size_t active_count = 0;
      for (const auto& tab : window.tabs) {
        if (!valid_identity(tab.tab_id) || !tab_ids.insert(tab.tab_id).second ||
            !safe_restoration_url(tab.url) || !valid_title(tab.title)) {
          return false;
        }
        if (tab.active) ++active_count;
      }
      if (active_count > 1) return false;
      if (window.active_tab_id.has_value()) {
        const auto* active = find_tab(window, *window.active_tab_id);
        if (active == nullptr || !active->active || active_count != 1) return false;
      } else if (active_count != 0) {
        return false;
      }
    }

    std::unordered_set<std::string> retired_window_ids;
    for (const auto& retired : result.retired_window_ids) {
      if (!valid_identity(retired) || window_ids.contains(retired) ||
          !retired_window_ids.insert(retired).second) {
        return false;
      }
    }

    std::unordered_set<std::string> retired_tab_ids;
    for (const auto& retired : result.retired_tab_ids) {
      if (!valid_identity(retired) || tab_ids.contains(retired) ||
          !retired_tab_ids.insert(retired).second) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] static bool valid_checkpoint_structure(
      const NormalSessionCheckpoint& checkpoint) {
    if (checkpoint.schema_version != kNormalSessionJournalSchemaVersion ||
        !valid_identity(checkpoint.checkpoint_id) ||
        !valid_identity(checkpoint.journal_id) ||
        !valid_identity(checkpoint.profile_id) ||
        checkpoint.privacy_context_id != kNormalPrivacyContextId ||
        !valid_identity(checkpoint.session_epoch) ||
        checkpoint.created_unix_ms == 0 ||
        checkpoint.retired_window_ids.size() > kMaxNormalSessionRetiredIds ||
        checkpoint.retired_tab_ids.size() > kMaxNormalSessionRetiredIds) {
      return false;
    }

    NormalSessionReplayResult projected;
    projected.accepted = true;
    projected.journal_id = checkpoint.journal_id;
    projected.profile_id = checkpoint.profile_id;
    projected.privacy_context_id = checkpoint.privacy_context_id;
    projected.session_epoch = checkpoint.session_epoch;
    projected.journal_high_water_mark = checkpoint.journal_high_water_mark;
    projected.lifecycle_state = checkpoint.lifecycle_state;
    projected.windows = checkpoint.windows;
    projected.retired_window_ids = checkpoint.retired_window_ids;
    projected.retired_tab_ids = checkpoint.retired_tab_ids;
    return valid_replayed_state(projected);
  }

  static void append_field(std::ostringstream& out, std::string_view value) {
    out << value.size() << ':' << value << ';';
  }

  [[nodiscard]] static std::string canonical_checkpoint_bytes(
      const NormalSessionCheckpoint& checkpoint) {
    std::ostringstream out;
    out << checkpoint.schema_version << '|'
        << checkpoint.created_unix_ms << '|'
        << checkpoint.journal_high_water_mark << '|'
        << static_cast<int>(checkpoint.lifecycle_state) << '|';
    append_field(out, checkpoint.checkpoint_id);
    append_field(out, checkpoint.journal_id);
    append_field(out, checkpoint.profile_id);
    append_field(out, checkpoint.privacy_context_id);
    append_field(out, checkpoint.session_epoch);

    out << checkpoint.windows.size() << '|';
    for (const auto& window : checkpoint.windows) {
      append_field(out, window.window_id);
      out << static_cast<int>(window.privacy_mode) << '|';
      append_field(out, window.active_tab_id.value_or(""));
      out << window.tabs.size() << '|';
      for (const auto& tab : window.tabs) {
        append_field(out, tab.tab_id);
        append_field(out, tab.url);
        append_field(out, tab.title);
        append_field(out, tab.workspace_id);
        append_field(out, tab.group_id.value_or(""));
        append_field(out, tab.split_id.value_or(""));
        out << (tab.pinned ? 1 : 0) << '|'
            << (tab.active ? 1 : 0) << '|'
            << tab.last_active_unix_ms << '|';
      }
    }

    out << checkpoint.retired_window_ids.size() << '|';
    for (const auto& value : checkpoint.retired_window_ids) append_field(out, value);
    out << checkpoint.retired_tab_ids.size() << '|';
    for (const auto& value : checkpoint.retired_tab_ids) append_field(out, value);
    return out.str();
  }

  [[nodiscard]] static std::uint64_t fnv1a64(std::string_view bytes) {
    // FNV-1a is used only as a deterministic corruption-detection checksum.
    // It is not an authenticity primitive and must not be treated as a MAC.
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : bytes) {
      hash ^= byte;
      hash *= 1099511628211ULL;
    }
    return hash;
  }

  [[nodiscard]] static std::string checksum_for(
      const NormalSessionCheckpoint& checkpoint) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0')
        << fnv1a64(canonical_checkpoint_bytes(checkpoint));
    return out.str();
  }
};

}  // namespace goreecloud::browser
