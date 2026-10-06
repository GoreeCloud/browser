#pragma once

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/normal_session_journal.hpp"

namespace goreecloud::browser {

enum class NormalSessionStartupClassification {
  no_durable_state,
  clean_shutdown,
  abnormal_termination,
  unavailable,
};

enum class NormalSessionRuntimeHealth {
  inactive,
  healthy,
  recovery_pending,
  storage_unavailable,
  transition_rejected,
  journal_write_failed,
  checkpoint_write_failed,
  clean_shutdown,
};

struct NormalSessionRuntimeOptions {
  std::string journal_id;
  std::string profile_id{"default"};
  std::string session_epoch;
  std::string checkpoint_id_prefix{"checkpoint"};
  std::size_t checkpoint_every_mutations{32};
};

struct NormalSessionRuntimeStatus {
  NormalSessionRuntimeHealth health{NormalSessionRuntimeHealth::inactive};
  NormalSessionStartupClassification startup{
      NormalSessionStartupClassification::no_durable_state};
  std::uint64_t committed_sequence{0};
  std::size_t successful_checkpoints{0};
  bool durable_state_current{false};
};

class NormalSessionRuntimeObserver {
 public:
  virtual ~NormalSessionRuntimeObserver() = default;

  [[nodiscard]] virtual bool normal_window_opened(std::string_view window_id) = 0;
  [[nodiscard]] virtual bool normal_window_closed(std::string_view window_id) = 0;
  [[nodiscard]] virtual bool normal_tab_opened(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view url,
      std::string_view title,
      std::size_t position) = 0;
  [[nodiscard]] virtual bool normal_tab_selected(
      std::string_view window_id,
      std::string_view tab_id) = 0;
  [[nodiscard]] virtual bool normal_tab_closed(
      std::string_view window_id,
      std::string_view tab_id) = 0;
  [[nodiscard]] virtual bool normal_tab_navigated(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view url) = 0;
  [[nodiscard]] virtual bool normal_tab_titled(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view title) = 0;
  [[nodiscard]] virtual bool normal_tab_reordered(
      std::string_view window_id,
      std::string_view tab_id,
      std::size_t position) = 0;
};

class NormalSessionRuntimeCoordinator final : public NormalSessionRuntimeObserver {
 public:
  using Clock = std::function<std::uint64_t()>;

  NormalSessionRuntimeCoordinator(
      NormalSessionDurableStore& store,
      NormalSessionRuntimeOptions options,
      Clock clock = {})
      : store_(store),
        options_(std::move(options)),
        clock_(clock ? std::move(clock) : Clock{default_now}) {}

  [[nodiscard]] NormalSessionStartupClassification classify_startup() const {
    const auto replayed = store_.replay_from_disk();
    if (!replayed.accepted) {
      return NormalSessionStartupClassification::unavailable;
    }
    if (replayed.journal_high_water_mark == 0 &&
        replayed.journal_id.empty() &&
        replayed.profile_id.empty() &&
        replayed.session_epoch.empty() &&
        replayed.windows.empty() &&
        replayed.retired_window_ids.empty() &&
        replayed.retired_tab_ids.empty()) {
      return NormalSessionStartupClassification::no_durable_state;
    }
    if (replayed.lifecycle_state == NormalSessionLifecycleState::clean_shutdown) {
      return NormalSessionStartupClassification::clean_shutdown;
    }
    return NormalSessionStartupClassification::abnormal_termination;
  }

  [[nodiscard]] bool begin() {
    if (status_.health != NormalSessionRuntimeHealth::inactive) {
      return status_.health == NormalSessionRuntimeHealth::healthy;
    }
    if (!valid_options()) {
      status_.health = NormalSessionRuntimeHealth::transition_rejected;
      return false;
    }

    status_.startup = classify_startup();
    if (status_.startup == NormalSessionStartupClassification::unavailable) {
      status_.health = NormalSessionRuntimeHealth::storage_unavailable;
      return false;
    }
    if (status_.startup == NormalSessionStartupClassification::abnormal_termination) {
      status_.health = NormalSessionRuntimeHealth::recovery_pending;
      return false;
    }
    if (status_.startup == NormalSessionStartupClassification::clean_shutdown &&
        !store_.erase_all()) {
      status_.health = NormalSessionRuntimeHealth::storage_unavailable;
      return false;
    }

    status_.health = NormalSessionRuntimeHealth::healthy;
    if (!record_lifecycle(NormalSessionLifecycleState::starting, false) ||
        !record_lifecycle(NormalSessionLifecycleState::running, false)) {
      return false;
    }
    return true;
  }

  [[nodiscard]] bool background() {
    if (!record_lifecycle(NormalSessionLifecycleState::backgrounding, false)) {
      return false;
    }
    return checkpoint_cycle(NormalSessionLifecycleState::backgrounding);
  }

  [[nodiscard]] bool resume() {
    return record_lifecycle(NormalSessionLifecycleState::running, false);
  }

  [[nodiscard]] bool clean_shutdown() {
    if (status_.health == NormalSessionRuntimeHealth::clean_shutdown) return true;
    if (status_.health != NormalSessionRuntimeHealth::healthy) return false;

    // A durable CLEAN_SHUTDOWN marker suppresses crash recovery on the next
    // startup, so it must be the final committed operation. Checkpoint first;
    // if checkpointing or the final append fails, durable state remains
    // non-clean and startup classifies it as abnormal.
    if (!checkpoint_cycle(NormalSessionLifecycleState::clean_shutdown)) {
      return false;
    }
    status_.health = NormalSessionRuntimeHealth::clean_shutdown;
    status_.durable_state_current = true;
    return true;
  }

  [[nodiscard]] bool normal_window_opened(std::string_view window_id) override {
    auto entry = base_entry(NormalSessionJournalOperation::open_window);
    entry.window_id = std::string{window_id};
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_window_closed(std::string_view window_id) override {
    auto entry = base_entry(NormalSessionJournalOperation::close_window);
    entry.window_id = std::string{window_id};
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_tab_opened(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view url,
      std::string_view title,
      std::size_t position) override {
    auto entry = base_entry(NormalSessionJournalOperation::open_tab);
    entry.window_id = std::string{window_id};
    entry.tab_id = std::string{tab_id};
    entry.url = durable_url(url);
    entry.title = durable_title(title);
    entry.position = position;
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_tab_selected(
      std::string_view window_id,
      std::string_view tab_id) override {
    auto entry = base_entry(NormalSessionJournalOperation::select_tab);
    entry.window_id = std::string{window_id};
    entry.tab_id = std::string{tab_id};
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_tab_closed(
      std::string_view window_id,
      std::string_view tab_id) override {
    auto entry = base_entry(NormalSessionJournalOperation::close_tab);
    entry.window_id = std::string{window_id};
    entry.tab_id = std::string{tab_id};
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_tab_navigated(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view url) override {
    auto entry = base_entry(NormalSessionJournalOperation::navigate_tab);
    entry.window_id = std::string{window_id};
    entry.tab_id = std::string{tab_id};
    entry.url = durable_url(url);
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_tab_titled(
      std::string_view window_id,
      std::string_view tab_id,
      std::string_view title) override {
    auto entry = base_entry(NormalSessionJournalOperation::title_tab);
    entry.window_id = std::string{window_id};
    entry.tab_id = std::string{tab_id};
    entry.title = durable_title(title);
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] bool normal_tab_reordered(
      std::string_view window_id,
      std::string_view tab_id,
      std::size_t position) override {
    auto entry = base_entry(NormalSessionJournalOperation::reorder_tab);
    entry.window_id = std::string{window_id};
    entry.tab_id = std::string{tab_id};
    entry.position = position;
    return record_mutation(std::move(entry));
  }

  [[nodiscard]] const NormalSessionRuntimeStatus& status() const noexcept {
    return status_;
  }

  [[nodiscard]] bool accepting_runtime_events() const noexcept {
    return status_.health == NormalSessionRuntimeHealth::healthy;
  }

 private:
  [[nodiscard]] static std::uint64_t default_now() {
    using namespace std::chrono;
    return static_cast<std::uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
  }

  [[nodiscard]] bool valid_options() const {
    return !options_.journal_id.empty() &&
           options_.journal_id.size() <= kMaxRecoveryIdentifierBytes &&
           !options_.profile_id.empty() &&
           options_.profile_id.size() <= kMaxRecoveryIdentifierBytes &&
           !options_.session_epoch.empty() &&
           options_.session_epoch.size() <= kMaxRecoveryIdentifierBytes &&
           !options_.checkpoint_id_prefix.empty() &&
           options_.checkpoint_id_prefix.size() <= kMaxRecoveryIdentifierBytes;
  }

  [[nodiscard]] NormalSessionJournalEntry base_entry(
      NormalSessionJournalOperation operation) const {
    NormalSessionJournalEntry entry;
    entry.journal_id = options_.journal_id;
    entry.profile_id = options_.profile_id;
    entry.privacy_context_id = std::string{kNormalPrivacyContextId};
    entry.session_epoch = options_.session_epoch;
    entry.sequence = next_sequence_;
    entry.created_unix_ms = clock_();
    entry.operation = operation;
    return entry;
  }

  [[nodiscard]] bool record_lifecycle(
      NormalSessionLifecycleState lifecycle,
      bool count_for_checkpoint) {
    auto entry = base_entry(NormalSessionJournalOperation::lifecycle);
    entry.lifecycle_state = lifecycle;
    return append_if_changed(std::move(entry), count_for_checkpoint);
  }

  [[nodiscard]] bool record_mutation(NormalSessionJournalEntry entry) {
    if (!append_if_changed(std::move(entry), true)) return false;
    if (options_.checkpoint_every_mutations > 0 &&
        mutations_since_checkpoint_ >= options_.checkpoint_every_mutations) {
      return checkpoint_cycle(NormalSessionLifecycleState::running);
    }
    return true;
  }

  [[nodiscard]] bool append_if_changed(
      NormalSessionJournalEntry entry,
      bool count_for_checkpoint) {
    if (status_.health != NormalSessionRuntimeHealth::healthy) return false;
    if (entry.created_unix_ms == 0) {
      status_.health = NormalSessionRuntimeHealth::transition_rejected;
      status_.durable_state_current = false;
      return false;
    }

    const auto candidate = replay_candidate(entry);
    if (!candidate.accepted) {
      status_.health = NormalSessionRuntimeHealth::transition_rejected;
      status_.durable_state_current = false;
      return false;
    }

    if (committed_state_.has_value() &&
        same_projection(*committed_state_, candidate)) {
      return true;
    }

    if (!store_.append(entry)) {
      status_.health = NormalSessionRuntimeHealth::journal_write_failed;
      status_.durable_state_current = false;
      return false;
    }

    committed_state_ = candidate;
    ++next_sequence_;
    status_.committed_sequence = entry.sequence;
    status_.durable_state_current = true;
    if (count_for_checkpoint) ++mutations_since_checkpoint_;
    return true;
  }

  [[nodiscard]] NormalSessionReplayResult replay_candidate(
      const NormalSessionJournalEntry& entry) const {
    if (!committed_state_.has_value()) {
      return NormalSessionJournalPolicy::replay(
          std::nullopt, std::vector<NormalSessionJournalEntry>{entry});
    }

    const auto shadow = NormalSessionJournalPolicy::checkpoint_from_replay(
        *committed_state_, "runtime-shadow", std::max<std::uint64_t>(1, entry.created_unix_ms));
    if (!shadow.has_value()) {
      NormalSessionReplayResult rejected;
      rejected.error = "runtime shadow checkpoint unavailable";
      return rejected;
    }
    return NormalSessionJournalPolicy::replay(
        *shadow, std::vector<NormalSessionJournalEntry>{entry});
  }

  [[nodiscard]] bool checkpoint_cycle(
      NormalSessionLifecycleState return_state) {
    if (status_.health != NormalSessionRuntimeHealth::healthy) return false;
    if (!record_lifecycle(NormalSessionLifecycleState::checkpointing, false)) {
      return false;
    }

    const auto checkpoint = make_checkpoint();
    if (!checkpoint.has_value() || !store_.compact(*checkpoint)) {
      status_.health = NormalSessionRuntimeHealth::checkpoint_write_failed;
      status_.durable_state_current = false;
      return false;
    }

    ++status_.successful_checkpoints;
    mutations_since_checkpoint_ = 0;
    return record_lifecycle(return_state, false);
  }

  [[nodiscard]] std::optional<NormalSessionCheckpoint> make_checkpoint() {
    if (!committed_state_.has_value()) return std::nullopt;
    const auto id = options_.checkpoint_id_prefix + "-" +
                    std::to_string(status_.successful_checkpoints + 1);
    if (id.size() > kMaxRecoveryIdentifierBytes) return std::nullopt;
    return NormalSessionJournalPolicy::checkpoint_from_replay(
        *committed_state_, id, std::max<std::uint64_t>(1, clock_()));
  }

  [[nodiscard]] static std::string durable_url(std::string_view url) {
    if (NormalSessionJournalPolicy::safe_restoration_url(url)) {
      return std::string{url};
    }
    return {};
  }

  [[nodiscard]] static std::string durable_title(std::string_view title) {
    std::string result;
    result.reserve(std::min<std::size_t>(title.size(), kMaxNormalSessionTitleBytes));
    for (const unsigned char ch : title) {
      if (result.size() >= kMaxNormalSessionTitleBytes) break;
      if (!std::iscntrl(ch)) result.push_back(static_cast<char>(ch));
    }
    return result;
  }

  [[nodiscard]] static bool same_tab(
      const RecoverableTab& left,
      const RecoverableTab& right) {
    return left.tab_id == right.tab_id &&
           left.url == right.url &&
           left.title == right.title &&
           left.workspace_id == right.workspace_id &&
           left.group_id == right.group_id &&
           left.split_id == right.split_id &&
           left.pinned == right.pinned &&
           left.active == right.active &&
           left.last_active_unix_ms == right.last_active_unix_ms;
  }

  [[nodiscard]] static bool same_window(
      const RecoverableWindow& left,
      const RecoverableWindow& right) {
    if (left.window_id != right.window_id ||
        left.privacy_mode != right.privacy_mode ||
        left.active_tab_id != right.active_tab_id ||
        left.tabs.size() != right.tabs.size()) {
      return false;
    }
    for (std::size_t index = 0; index < left.tabs.size(); ++index) {
      if (!same_tab(left.tabs[index], right.tabs[index])) return false;
    }
    return true;
  }

  [[nodiscard]] static bool same_projection(
      const NormalSessionReplayResult& left,
      const NormalSessionReplayResult& right) {
    if (left.journal_id != right.journal_id ||
        left.profile_id != right.profile_id ||
        left.privacy_context_id != right.privacy_context_id ||
        left.session_epoch != right.session_epoch ||
        left.lifecycle_state != right.lifecycle_state ||
        left.retired_window_ids != right.retired_window_ids ||
        left.retired_tab_ids != right.retired_tab_ids ||
        left.windows.size() != right.windows.size()) {
      return false;
    }
    for (std::size_t index = 0; index < left.windows.size(); ++index) {
      if (!same_window(left.windows[index], right.windows[index])) return false;
    }
    return true;
  }

  NormalSessionDurableStore& store_;
  NormalSessionRuntimeOptions options_;
  Clock clock_;
  NormalSessionRuntimeStatus status_;
  std::optional<NormalSessionReplayResult> committed_state_;
  std::uint64_t next_sequence_{1};
  std::size_t mutations_since_checkpoint_{0};
};

}  // namespace goreecloud::browser
