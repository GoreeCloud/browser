#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "goreecloud/browser/application.hpp"
#include "goreecloud/browser/development_engine.hpp"
#include "goreecloud/browser/normal_session_runtime.hpp"

namespace {

void require(bool condition) {
  if (!condition) std::abort();
}

class MemoryNormalSessionStore final
    : public goreecloud::browser::NormalSessionDurableStore {
 public:
  bool append(
      const goreecloud::browser::NormalSessionJournalEntry& entry) override {
    if (fail_append_sequence.has_value() &&
        entry.sequence == *fail_append_sequence) {
      return false;
    }
    journal.push_back(entry);
    history.push_back(entry);
    return true;
  }

  bool compact(
      const goreecloud::browser::NormalSessionCheckpoint& value) override {
    if (fail_compact) return false;
    checkpoint = value;
    journal.erase(
        std::remove_if(
            journal.begin(),
            journal.end(),
            [&](const auto& entry) {
              return entry.sequence <= value.journal_high_water_mark;
            }),
        journal.end());
    ++compact_count;
    return true;
  }

  goreecloud::browser::NormalSessionReplayResult replay_from_disk()
      const override {
    return goreecloud::browser::NormalSessionJournalPolicy::replay(
        checkpoint, journal);
  }

  bool erase_all() override {
    if (fail_erase) return false;
    checkpoint.reset();
    journal.clear();
    return true;
  }

  std::optional<std::uint64_t> fail_append_sequence;
  bool fail_compact{false};
  bool fail_erase{false};
  std::size_t compact_count{0};
  std::optional<goreecloud::browser::NormalSessionCheckpoint> checkpoint;
  std::vector<goreecloud::browser::NormalSessionJournalEntry> journal;
  std::vector<goreecloud::browser::NormalSessionJournalEntry> history;
};

goreecloud::browser::NormalSessionRuntimeOptions options(
    std::string epoch,
    std::size_t checkpoint_every = 0) {
  return goreecloud::browser::NormalSessionRuntimeOptions{
      .journal_id = "journal-" + epoch,
      .profile_id = "default",
      .session_epoch = std::move(epoch),
      .checkpoint_id_prefix = "checkpoint",
      .checkpoint_every_mutations = checkpoint_every,
  };
}

auto deterministic_clock() {
  return [now = std::uint64_t{1'800'200'000'000ULL}]() mutable {
    return now++;
  };
}

void require_contiguous(
    const std::vector<goreecloud::browser::NormalSessionJournalEntry>& entries) {
  for (std::size_t index = 0; index < entries.size(); ++index) {
    require(entries[index].sequence == index + 1);
  }
}

}  // namespace

int main() {
  using namespace goreecloud::browser;

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("direct"), deterministic_clock());

    require(
        runtime.classify_startup() ==
        NormalSessionStartupClassification::no_durable_state);
    require(runtime.begin());
    require(runtime.status().health == NormalSessionRuntimeHealth::healthy);
    require(runtime.status().committed_sequence == 2);

    require(runtime.normal_window_opened("window-1"));
    require(runtime.normal_tab_opened(
        "window-1", "tab-1", "https://example.com/", "Example", 0));
    require(runtime.normal_tab_selected("window-1", "tab-1"));

    const auto before_duplicate = store.history.size();
    require(runtime.normal_tab_selected("window-1", "tab-1"));
    require(runtime.normal_tab_titled("window-1", "tab-1", "Example"));
    require(runtime.normal_tab_navigated(
        "window-1", "tab-1", "https://example.com/"));
    require(store.history.size() == before_duplicate);

    require(runtime.normal_tab_closed("window-1", "tab-1"));
    const auto before_late = store.history.size();
    require(runtime.normal_tab_navigated(
        "window-1", "tab-1", "https://example.com/late"));
    require(runtime.normal_tab_titled("window-1", "tab-1", "Late"));
    require(store.history.size() == before_late);

    require(runtime.background());
    require(runtime.status().successful_checkpoints == 1);
    require(runtime.status().health == NormalSessionRuntimeHealth::healthy);
    require(runtime.resume());
    require(runtime.clean_shutdown());
    require(runtime.status().health == NormalSessionRuntimeHealth::clean_shutdown);
    require(runtime.status().durable_state_current);
    require_contiguous(store.history);
    require(store.compact_count == 2);
    require(
        runtime.classify_startup() ==
        NormalSessionStartupClassification::clean_shutdown);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator first(
        store, options("clean-old"), deterministic_clock());
    require(first.begin());
    require(first.normal_window_opened("window-old"));
    require(first.clean_shutdown());

    NormalSessionRuntimeCoordinator next(
        store, options("clean-new"), deterministic_clock());
    require(
        next.classify_startup() ==
        NormalSessionStartupClassification::clean_shutdown);
    require(next.begin());
    require(next.status().health == NormalSessionRuntimeHealth::healthy);
    require(next.status().startup == NormalSessionStartupClassification::clean_shutdown);
    require(next.status().committed_sequence == 2);
    require(store.journal.size() == 2);
    require(store.journal.front().session_epoch == "clean-new");
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator first(
        store, options("crashed"), deterministic_clock());
    require(first.begin());
    require(first.normal_window_opened("window-crashed"));

    NormalSessionRuntimeCoordinator after_crash(
        store, options("after-crash"), deterministic_clock());
    require(
        after_crash.classify_startup() ==
        NormalSessionStartupClassification::abnormal_termination);
    require(!after_crash.begin());
    require(
        after_crash.status().health ==
        NormalSessionRuntimeHealth::recovery_pending);
    require(store.history.size() == 3);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator first(
        store, options("recoverable"), deterministic_clock());
    require(first.begin());
    require(first.normal_window_opened("window-recovered"));
    require(first.normal_tab_opened(
        "window-recovered",
        "tab-recovered",
        "https://example.com/recovered",
        "Recovered",
        0));

    NormalSessionRuntimeCoordinator resumed(
        store, options("recoverable"), deterministic_clock());
    const auto recovery = resumed.recoverable_state();
    require(recovery.has_value());
    require(recovery->journal_high_water_mark == 4);
    require(recovery->windows.size() == 1);
    require(recovery->windows.front().tabs.size() == 1);
    require(resumed.resume_recovery());
    require(resumed.status().health == NormalSessionRuntimeHealth::healthy);
    require(
        resumed.status().startup ==
        NormalSessionStartupClassification::abnormal_termination);
    require(resumed.status().committed_sequence == 6);
    require(store.history.size() == 6);
    require_contiguous(store.history);
    require(
        store.history[4].lifecycle_state ==
        NormalSessionLifecycleState::restoring);
    require(
        store.history[5].lifecycle_state ==
        NormalSessionLifecycleState::running);

    NormalSessionRuntimeCoordinator wrong_epoch(
        store, options("different-epoch"), deterministic_clock());
    require(!wrong_epoch.resume_recovery());
    require(
        wrong_epoch.status().health ==
        NormalSessionRuntimeHealth::transition_rejected);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator first(
        store, options("interrupted-recovery"), deterministic_clock());
    require(first.begin());
    require(first.normal_window_opened("window-recovered"));
    require(first.normal_tab_opened(
        "window-recovered",
        "tab-recovered",
        "https://example.com/recovered",
        "Recovered",
        0));

    store.fail_append_sequence = 6;
    NormalSessionRuntimeCoordinator interrupted(
        store, options("interrupted-recovery"), deterministic_clock());
    require(!interrupted.resume_recovery());
    require(
        interrupted.status().health ==
        NormalSessionRuntimeHealth::journal_write_failed);
    require(interrupted.status().committed_sequence == 5);
    require(
        interrupted.classify_startup() ==
        NormalSessionStartupClassification::abnormal_termination);

    store.fail_append_sequence.reset();
    NormalSessionRuntimeCoordinator retry(
        store, options("interrupted-recovery"), deterministic_clock());
    require(retry.resume_recovery());
    require(retry.status().committed_sequence == 6);
    require(store.history.size() == 6);
    require_contiguous(store.history);
    require(
        store.history.back().lifecycle_state ==
        NormalSessionLifecycleState::running);
  }

  {
    MemoryNormalSessionStore store;
    store.fail_append_sequence = 4;
    NormalSessionRuntimeCoordinator runtime(
        store, options("append-failure"), deterministic_clock());
    require(runtime.begin());
    require(runtime.normal_window_opened("window-1"));
    require(!runtime.normal_tab_opened(
        "window-1", "tab-1", "https://example.com/", "Example", 0));
    require(
        runtime.status().health ==
        NormalSessionRuntimeHealth::journal_write_failed);
    require(runtime.status().committed_sequence == 3);
    require(!runtime.status().durable_state_current);
    require(store.history.size() == 3);
    require_contiguous(store.history);
    require(!runtime.normal_window_opened("window-2"));
    require(store.history.size() == 3);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("tab-close-failure"), deterministic_clock());
    BrowserApplication browser(
        std::make_unique<DevelopmentEngine>(),
        BrowserApplicationOptions{},
        &runtime);

    browser.initialize();
    auto* window = browser.first_window();
    require(window);
    auto* tab = window->active_tab();
    require(tab);
    const auto tab_id = tab->id();
    const auto tab_count = window->tab_count();

    store.fail_append_sequence = runtime.status().committed_sequence + 1;
    require(!window->close_tab(tab_id));
    require(window->tab_count() == tab_count);
    require(window->active_tab());
    require(window->active_tab()->id() == tab_id);
    require(
        runtime.status().health ==
        NormalSessionRuntimeHealth::journal_write_failed);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("window-close-failure"), deterministic_clock());
    BrowserApplication browser(
        std::make_unique<DevelopmentEngine>(),
        BrowserApplicationOptions{},
        &runtime);

    browser.initialize();
    require(browser.first_window());
    const auto window_id = browser.first_window()->window_id();
    const auto window_count = browser.window_count();

    store.fail_append_sequence = runtime.status().committed_sequence + 1;
    require(!browser.close_window(window_id));
    require(browser.window_count() == window_count);
    require(browser.first_window());
    require(browser.first_window()->window_id() == window_id);
    require(
        runtime.status().health ==
        NormalSessionRuntimeHealth::journal_write_failed);
  }

  {
    MemoryNormalSessionStore store;
    store.fail_compact = true;
    NormalSessionRuntimeCoordinator runtime(
        store, options("checkpoint-failure", 1), deterministic_clock());
    require(runtime.begin());
    require(!runtime.normal_window_opened("window-1"));
    require(
        runtime.status().health ==
        NormalSessionRuntimeHealth::checkpoint_write_failed);
    require(!runtime.status().durable_state_current);
    require(store.history.size() == 4);
    require_contiguous(store.history);
    require(
        store.history.back().lifecycle_state ==
        NormalSessionLifecycleState::checkpointing);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("clean-checkpoint-failure"), deterministic_clock());
    require(runtime.begin());
    require(runtime.normal_window_opened("window-1"));
    store.fail_compact = true;
    require(!runtime.clean_shutdown());
    require(
        runtime.status().health ==
        NormalSessionRuntimeHealth::checkpoint_write_failed);
    require(
        runtime.classify_startup() ==
        NormalSessionStartupClassification::abnormal_termination);
    require(!store.history.empty());
    require(
        store.history.back().lifecycle_state ==
        NormalSessionLifecycleState::checkpointing);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("clean-marker-failure"), deterministic_clock());
    require(runtime.begin());
    require(runtime.normal_window_opened("window-1"));
    store.fail_append_sequence = 5;
    require(!runtime.clean_shutdown());
    require(
        runtime.status().health ==
        NormalSessionRuntimeHealth::journal_write_failed);
    require(
        runtime.classify_startup() ==
        NormalSessionStartupClassification::abnormal_termination);
    require(store.checkpoint.has_value());
    require(
        store.checkpoint->lifecycle_state ==
        NormalSessionLifecycleState::checkpointing);
    require(store.history.size() == 4);
    require_contiguous(store.history);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("app"), deterministic_clock());
    BrowserApplication browser(
        std::make_unique<DevelopmentEngine>(),
        BrowserApplicationOptions{},
        &runtime);

    browser.initialize();
    require(browser.normal_session_status());
    require(
        browser.normal_session_status()->health ==
        NormalSessionRuntimeHealth::healthy);
    require(browser.window_count() == 1);
    require(browser.first_window());
    require(browser.first_window()->active_tab());

    const auto normal_history = store.history.size();
    auto& private_window = browser.new_private_window("private-runtime");
    (void)private_window.new_tab("https://private.example/");
    require(store.history.size() == normal_history);

    auto& second_window = browser.new_window(false);
    auto& second_tab = second_window.new_tab("https://second.example/");
    second_window.navigate_active("https://second.example/updated");
    browser.pump_events();
    require(
        second_tab.engine_view().navigation_state().url ==
        "https://second.example/updated");

    const auto second_window_id = second_window.window_id();
    require(browser.close_window(second_window_id));
    require(browser.window_count() == 2);

    browser.background_normal_session();
    require(runtime.status().successful_checkpoints >= 1);
    browser.resume_normal_session();
    browser.shutdown();

    require(runtime.status().health == NormalSessionRuntimeHealth::clean_shutdown);
    require(runtime.status().durable_state_current);
    require_contiguous(store.history);

    const auto replayed = store.replay_from_disk();
    require(replayed.accepted);
    require(replayed.lifecycle_state == NormalSessionLifecycleState::clean_shutdown);
    require(replayed.windows.size() == 1);
    require(replayed.windows.front().privacy_mode == SessionPrivacyMode::normal);
  }

  {
    MemoryNormalSessionStore store;
    NormalSessionRuntimeCoordinator runtime(
        store, options("private-only"), deterministic_clock());
    BrowserApplicationOptions private_options;
    private_options.initial_private_window = true;
    private_options.initial_private_session_id = "private-only";
    private_options.initial_url = "https://private.example/";

    BrowserApplication browser(
        std::make_unique<DevelopmentEngine>(), private_options, &runtime);
    browser.initialize();
    require(store.history.empty());
    require(runtime.status().health == NormalSessionRuntimeHealth::inactive);
    browser.shutdown();
    require(store.history.empty());
  }

  return 0;
}
