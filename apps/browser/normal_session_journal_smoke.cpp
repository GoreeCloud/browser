#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "goreecloud/browser/normal_session_journal.hpp"

namespace {

[[noreturn]] void fail_requirement(const char* expression, int line) {
  std::cerr << "Requirement failed at line " << line << ": " << expression << '\n';
  std::abort();
}

#define REQUIRE(expression) \
  do { \
    if (!(expression)) fail_requirement(#expression, __LINE__); \
  } while (false)


void require(bool condition) {
  if (!condition) std::abort();
}

goreecloud::browser::NormalSessionJournalEntry entry(
    std::uint64_t sequence,
    goreecloud::browser::NormalSessionJournalOperation operation) {
  using namespace goreecloud::browser;
  NormalSessionJournalEntry value;
  value.journal_id = "journal-normal-001";
  value.profile_id = "profile-default";
  value.privacy_context_id = std::string{kNormalPrivacyContextId};
  value.session_epoch = "epoch-001";
  value.sequence = sequence;
  value.created_unix_ms = 1'800'000'000'000ULL + sequence;
  value.operation = operation;
  return value;
}

goreecloud::browser::NormalSessionJournalEntry lifecycle(
    std::uint64_t sequence,
    goreecloud::browser::NormalSessionLifecycleState state) {
  auto value = entry(sequence, goreecloud::browser::NormalSessionJournalOperation::lifecycle);
  value.lifecycle_state = state;
  return value;
}

goreecloud::browser::NormalSessionJournalEntry open_window(std::uint64_t sequence) {
  auto value = entry(sequence, goreecloud::browser::NormalSessionJournalOperation::open_window);
  value.window_id = "window-main";
  return value;
}

goreecloud::browser::NormalSessionJournalEntry open_tab(
    std::uint64_t sequence,
    std::string tab_id,
    std::string url,
    std::string title,
    std::size_t position) {
  auto value = entry(sequence, goreecloud::browser::NormalSessionJournalOperation::open_tab);
  value.window_id = "window-main";
  value.tab_id = std::move(tab_id);
  value.url = std::move(url);
  value.title = std::move(title);
  value.position = position;
  return value;
}

goreecloud::browser::NormalSessionJournalEntry tab_event(
    std::uint64_t sequence,
    goreecloud::browser::NormalSessionJournalOperation operation,
    std::string tab_id) {
  auto value = entry(sequence, operation);
  value.window_id = "window-main";
  value.tab_id = std::move(tab_id);
  return value;
}

}  // namespace

int main() {
  using namespace goreecloud::browser;

  auto start = lifecycle(1, NormalSessionLifecycleState::starting);
  auto window = open_window(2);
  auto first = open_tab(3, "tab-1", "https://example.com/one", "One", 0);
  auto second = open_tab(4, "tab-2", "https://example.com/two", "Two", 1);

  auto select_first = tab_event(5, NormalSessionJournalOperation::select_tab, "tab-1");

  auto navigate_first = tab_event(6, NormalSessionJournalOperation::navigate_tab, "tab-1");
  navigate_first.url = "https://example.com/one/updated";

  auto title_first = tab_event(7, NormalSessionJournalOperation::title_tab, "tab-1");
  title_first.title = "Updated One";

  auto reorder_first = tab_event(8, NormalSessionJournalOperation::reorder_tab, "tab-1");
  reorder_first.position = 1;

  auto close_second = tab_event(9, NormalSessionJournalOperation::close_tab, "tab-2");

  // A late callback for a committed close is allowed to replay as a no-op and
  // must never recreate the closed tab.
  auto late_navigation = tab_event(10, NormalSessionJournalOperation::navigate_tab, "tab-2");
  late_navigation.url = "https://example.com/two/late";

  auto running = lifecycle(11, NormalSessionLifecycleState::running);

  std::vector<NormalSessionJournalEntry> shuffled{
      close_second,
      first,
      running,
      start,
      title_first,
      second,
      window,
      select_first,
      navigate_first,
      reorder_first,
      late_navigation,
      title_first,  // Exact duplicate sequence/input is idempotent.
  };
  std::reverse(shuffled.begin(), shuffled.end());

  auto replayed = NormalSessionJournalPolicy::replay(std::nullopt, shuffled);
  require(replayed.accepted);
  require(replayed.restore_eligible);
  require(replayed.inferred_abnormal_termination);
  require(replayed.journal_high_water_mark == 11);
  require(replayed.lifecycle_state == NormalSessionLifecycleState::running);
  require(replayed.windows.size() == 1);
  require(replayed.windows.front().window_id == "window-main");
  require(replayed.windows.front().tabs.size() == 1);
  require(replayed.windows.front().tabs.front().tab_id == "tab-1");
  require(replayed.windows.front().tabs.front().url == "https://example.com/one/updated");
  require(replayed.windows.front().tabs.front().title == "Updated One");
  require(replayed.windows.front().active_tab_id == "tab-1");
  require(replayed.windows.front().tabs.front().active);
  require(std::find(replayed.retired_tab_ids.begin(), replayed.retired_tab_ids.end(), "tab-2") !=
         replayed.retired_tab_ids.end());

  auto sealed = NormalSessionJournalPolicy::checkpoint_from_replay(
      replayed, "checkpoint-001", 1'800'000'100'000ULL);
  require(sealed.has_value());
  require(NormalSessionJournalPolicy::valid_checkpoint(*sealed));
  require(sealed->journal_high_water_mark == 11);
  require(sealed->retired_tab_ids.size() == 1);
  require(sealed->retired_tab_ids.front() == "tab-2");

  // Stale records at or below the checkpoint high-water mark are ignored even
  // when supplied again after compaction.
  auto clean = lifecycle(12, NormalSessionLifecycleState::clean_shutdown);
  auto clean_replay = NormalSessionJournalPolicy::replay(
      sealed, std::vector<NormalSessionJournalEntry>{second, clean});
  require(clean_replay.accepted);
  require(!clean_replay.restore_eligible);
  require(!clean_replay.inferred_abnormal_termination);
  require(clean_replay.lifecycle_state == NormalSessionLifecycleState::clean_shutdown);
  require(clean_replay.windows.front().tabs.size() == 1);

  auto post_shutdown_title = tab_event(
      13, NormalSessionJournalOperation::title_tab, "tab-1");
  post_shutdown_title.title = "Too late";
  auto post_shutdown_replay = NormalSessionJournalPolicy::replay(
      sealed,
      std::vector<NormalSessionJournalEntry>{clean, post_shutdown_title});
  require(!post_shutdown_replay.accepted);

  auto missing_initial = open_window(2);
  auto missing_initial_replay = NormalSessionJournalPolicy::replay(
      std::nullopt, std::vector<NormalSessionJournalEntry>{missing_initial});
  require(!missing_initial_replay.accepted);

  auto gap_after_checkpoint = lifecycle(
      13, NormalSessionLifecycleState::running);
  auto gap_replay = NormalSessionJournalPolicy::replay(
      sealed, std::vector<NormalSessionJournalEntry>{gap_after_checkpoint});
  require(!gap_replay.accepted);

  // Stable tab identifiers cannot be reused after a committed close, even
  // after compaction has removed the original open/close records.
  auto resurrection = open_tab(
      12, "tab-2", "https://example.com/resurrected", "Resurrected", 1);
  auto rejected_resurrection = NormalSessionJournalPolicy::replay(
      sealed, std::vector<NormalSessionJournalEntry>{resurrection});
  require(!rejected_resurrection.accepted);

  // Corrupt checkpoint bytes/integrity never become Browser state.
  auto corrupt = *sealed;
  corrupt.integrity_checksum.push_back('0');
  auto corrupt_replay = NormalSessionJournalPolicy::replay(
      corrupt, std::vector<NormalSessionJournalEntry>{});
  require(!corrupt_replay.accepted);

  // Unsupported durable schema versions fail closed.
  auto unsupported_checkpoint = *sealed;
  unsupported_checkpoint.schema_version = kNormalSessionJournalSchemaVersion + 1;
  auto unsupported_replay = NormalSessionJournalPolicy::replay(
      unsupported_checkpoint, std::vector<NormalSessionJournalEntry>{});
  require(!unsupported_replay.accepted);

  auto unsupported_entry = running;
  unsupported_entry.schema_version = kNormalSessionJournalSchemaVersion + 1;
  auto unsupported_journal = NormalSessionJournalPolicy::replay(
      std::nullopt, std::vector<NormalSessionJournalEntry>{unsupported_entry});
  require(!unsupported_journal.accepted);

  // Private/Isolated Private durable records are not accepted by this Normal
  // persistence contract.
  auto private_entry = start;
  private_entry.privacy_context_id = "private";
  auto private_replay = NormalSessionJournalPolicy::replay(
      std::nullopt, std::vector<NormalSessionJournalEntry>{private_entry});
  require(!private_replay.accepted);

  // Credential-bearing HTTP(S) locations must never enter the durable
  // restoration projection.
  auto credential_url = open_tab(
      3, "tab-secret", "https://user:password@example.com/private", "Secret", 0);
  auto credential_replay = NormalSessionJournalPolicy::replay(
      std::nullopt,
      std::vector<NormalSessionJournalEntry>{start, window, credential_url});
  require(!credential_replay.accepted);

  // Conflicting reuse of one journal sequence is corruption, not a last-write
  // wins update.
  auto conflicting_title = title_first;
  conflicting_title.title = "Different";
  auto conflict = NormalSessionJournalPolicy::replay(
      std::nullopt,
      std::vector<NormalSessionJournalEntry>{
          start,
          window,
          first,
          title_first,
          conflicting_title,
      });
  require(!conflict.accepted);

  // Checkpoint construction rejects impossible active-tab references before
  // the checkpoint can be sealed.
  auto impossible = *sealed;
  impossible.integrity_checksum.clear();
  impossible.windows.front().active_tab_id = "missing-tab";
  require(!NormalSessionJournalPolicy::seal_checkpoint(impossible).has_value());

  auto duplicate_tombstone = *sealed;
  duplicate_tombstone.integrity_checksum.clear();
  duplicate_tombstone.retired_tab_ids.push_back("tab-2");
  require(!NormalSessionJournalPolicy::seal_checkpoint(duplicate_tombstone).has_value());

  // Determinism: the same durable input in a different physical order yields
  // the same Browser-owned logical projection.
  auto reordered_input = shuffled;
  std::rotate(reordered_input.begin(), reordered_input.begin() + 3, reordered_input.end());
  auto replayed_again = NormalSessionJournalPolicy::replay(std::nullopt, reordered_input);
  require(replayed_again.accepted);
  require(replayed_again.journal_high_water_mark == replayed.journal_high_water_mark);
  require(replayed_again.windows.size() == replayed.windows.size());
  require(replayed_again.windows.front().tabs.size() == replayed.windows.front().tabs.size());
  require(replayed_again.windows.front().tabs.front().tab_id ==
         replayed.windows.front().tabs.front().tab_id);
  require(replayed_again.windows.front().tabs.front().url ==
         replayed.windows.front().tabs.front().url);
  require(replayed_again.retired_tab_ids == replayed.retired_tab_ids);

  return 0;
}
