#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "goreecloud/browser/file_normal_session_store.hpp"

namespace {

goreecloud::browser::NormalSessionJournalEntry base_entry(
    std::uint64_t sequence,
    goreecloud::browser::NormalSessionJournalOperation operation) {
  using namespace goreecloud::browser;
  return NormalSessionJournalEntry{
      .journal_id = "journal-file-001",
      .profile_id = "profile-default",
      .privacy_context_id = std::string{kNormalPrivacyContextId},
      .session_epoch = "epoch-file-001",
      .sequence = sequence,
      .created_unix_ms = 1'800'100'000'000ULL + sequence,
      .operation = operation,
  };
}

goreecloud::browser::NormalSessionJournalEntry lifecycle(
    std::uint64_t sequence,
    goreecloud::browser::NormalSessionLifecycleState state) {
  auto value = base_entry(
      sequence, goreecloud::browser::NormalSessionJournalOperation::lifecycle);
  value.lifecycle_state = state;
  return value;
}

goreecloud::browser::NormalSessionJournalEntry open_window(std::uint64_t sequence) {
  auto value = base_entry(
      sequence, goreecloud::browser::NormalSessionJournalOperation::open_window);
  value.window_id = "window-main";
  return value;
}

goreecloud::browser::NormalSessionJournalEntry open_tab(std::uint64_t sequence) {
  auto value = base_entry(
      sequence, goreecloud::browser::NormalSessionJournalOperation::open_tab);
  value.window_id = "window-main";
  value.tab_id = "tab-main";
  value.url = "https://example.com/start";
  value.title = "Start";
  value.position = 0;
  return value;
}

goreecloud::browser::NormalSessionJournalEntry tab_event(
    std::uint64_t sequence,
    goreecloud::browser::NormalSessionJournalOperation operation) {
  auto value = base_entry(sequence, operation);
  value.window_id = "window-main";
  value.tab_id = "tab-main";
  return value;
}

}  // namespace

int main() {
  using namespace goreecloud::browser;

  const auto root =
      std::filesystem::temp_directory_path() / "goreecloud-normal-session-store-smoke";
  std::error_code error;
  std::filesystem::remove_all(root, error);

  FileNormalSessionStore store(root);
  assert(store.append(lifecycle(1, NormalSessionLifecycleState::starting)));
  assert(store.append(open_window(2)));
  assert(store.append(open_tab(3)));
  assert(store.append(lifecycle(4, NormalSessionLifecycleState::running)));

  auto replayed = store.replay_from_disk();
  assert(replayed.accepted);
  assert(replayed.restore_eligible);
  assert(replayed.journal_high_water_mark == 4);
  assert(replayed.windows.size() == 1);
  assert(replayed.windows.front().tabs.size() == 1);
  assert(replayed.windows.front().tabs.front().tab_id == "tab-main");

  auto checkpoint = NormalSessionJournalPolicy::checkpoint_from_replay(
      replayed, "checkpoint-file-001", 1'800'100'100'000ULL);
  assert(checkpoint.has_value());

  auto title = tab_event(5, NormalSessionJournalOperation::title_tab);
  title.title = "After checkpoint capture";
  assert(store.append(title));

  // A structurally valid but semantically forged checkpoint may not advance
  // the durable high-water mark or trim journal records it does not represent.
  auto forged = *checkpoint;
  forged.integrity_checksum.clear();
  forged.windows.front().tabs.clear();
  forged.windows.front().active_tab_id.reset();
  const auto forged_sealed = NormalSessionJournalPolicy::seal_checkpoint(forged);
  assert(forged_sealed.has_value());
  assert(!store.compact(*forged_sealed));
  const auto untrimmed = store.read_journal();
  assert(untrimmed.has_value());
  assert(untrimmed->size() == 5);

  // Compaction writes the checkpoint first, then trims only records covered by
  // its high-water mark. A later committed record must remain in the journal.
  assert(store.compact(*checkpoint));
  const auto compacted_journal = store.read_journal();
  assert(compacted_journal.has_value());
  assert(compacted_journal->size() == 1);
  assert(compacted_journal->front().sequence == 5);

  replayed = store.replay_from_disk();
  assert(replayed.accepted);
  assert(replayed.journal_high_water_mark == 5);
  assert(replayed.windows.front().tabs.front().title == "After checkpoint capture");

  auto close = tab_event(6, NormalSessionJournalOperation::close_tab);
  assert(store.append(close));
  replayed = store.replay_from_disk();
  assert(replayed.accepted);
  assert(replayed.journal_high_water_mark == 6);
  assert(replayed.windows.front().tabs.empty());
  assert(std::find(
             replayed.retired_tab_ids.begin(),
             replayed.retired_tab_ids.end(),
             "tab-main") != replayed.retired_tab_ids.end());

  auto closed_checkpoint = NormalSessionJournalPolicy::checkpoint_from_replay(
      replayed, "checkpoint-file-closed", 1'800'100'200'000ULL);
  assert(closed_checkpoint.has_value());
  assert(store.compact(*closed_checkpoint));
  assert(store.read_journal().has_value());
  assert(store.read_journal()->empty());

  // Re-appending an old pre-checkpoint record is harmless because its sequence
  // is beneath the checkpoint high-water mark.
  auto stale_open = open_tab(3);
  assert(store.append(stale_open));
  replayed = store.replay_from_disk();
  assert(replayed.accepted);
  assert(replayed.windows.front().tabs.empty());

  // A new record may not reuse the retired stable tab identifier.
  auto resurrection = open_tab(7);
  resurrection.url = "https://example.com/resurrection";
  assert(store.append(resurrection));
  replayed = store.replay_from_disk();
  assert(!replayed.accepted);

  // Start a clean fixture for disk corruption checks.
  assert(store.erase_all());
  assert(store.append(lifecycle(1, NormalSessionLifecycleState::starting)));
  assert(store.append(open_window(2)));
  assert(store.append(open_tab(3)));
  assert(store.append(lifecycle(4, NormalSessionLifecycleState::running)));
  replayed = store.replay_from_disk();
  assert(replayed.accepted);

  checkpoint = NormalSessionJournalPolicy::checkpoint_from_replay(
      replayed, "checkpoint-file-corruption", 1'800'100'300'000ULL);
  assert(checkpoint.has_value());
  assert(store.compact(*checkpoint));

  // A partial append without a terminating newline is not a committed record.
  {
    std::ofstream out(
        root / "normal-session.journal",
        std::ios::binary | std::ios::app);
    assert(out);
    out << "J\tpartial";
  }
  assert(!store.read_journal().has_value());
  assert(!store.replay_from_disk().accepted);

  // Restore the valid journal/checkpoint pair, then corrupt the checkpoint
  // after its explicit end marker. Recovery must fail closed.
  assert(store.erase_all());
  assert(store.append(lifecycle(1, NormalSessionLifecycleState::starting)));
  assert(store.append(open_window(2)));
  assert(store.append(open_tab(3)));
  assert(store.append(lifecycle(4, NormalSessionLifecycleState::running)));
  replayed = store.replay_from_disk();
  assert(replayed.accepted);
  checkpoint = NormalSessionJournalPolicy::checkpoint_from_replay(
      replayed, "checkpoint-file-tail", 1'800'100'400'000ULL);
  assert(checkpoint.has_value());
  assert(store.compact(*checkpoint));

  {
    std::ofstream out(
        root / "normal-session.checkpoint",
        std::ios::binary | std::ios::app);
    assert(out);
    out << "unexpected\n";
  }
  assert(!store.read_checkpoint().has_value());
  assert(!store.replay_from_disk().accepted);

  // Syntax filtering happens before bytes reach the durable journal.
  assert(store.erase_all());

  auto private_record = lifecycle(1, NormalSessionLifecycleState::starting);
  private_record.privacy_context_id = "private";
  assert(!store.append(private_record));

  auto credential_record = open_tab(3);
  credential_record.url = "https://user:secret@example.com/";
  assert(!store.append(credential_record));

  auto unsupported = lifecycle(1, NormalSessionLifecycleState::starting);
  unsupported.schema_version = kNormalSessionJournalSchemaVersion + 1;
  assert(!store.append(unsupported));

  std::filesystem::remove_all(root, error);
  return 0;
}
