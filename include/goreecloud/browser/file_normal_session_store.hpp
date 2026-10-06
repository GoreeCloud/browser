#pragma once

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "goreecloud/browser/normal_session_journal.hpp"

namespace goreecloud::browser {

inline constexpr std::uintmax_t kMaxNormalSessionDurableFileBytes = 8U * 1024U * 1024U;

class FileNormalSessionStore final : public NormalSessionDurableStore {
 public:
  explicit FileNormalSessionStore(std::filesystem::path directory)
      : directory_(std::move(directory)) {}

  [[nodiscard]] bool append(const NormalSessionJournalEntry& entry) override {
    if (!NormalSessionJournalPolicy::valid_journal_entry(entry)) return false;

    std::error_code error;
    std::filesystem::create_directories(directory_, error);
    if (error) return false;

    const auto record = serialize_journal_record(entry);
    const auto path = journal_path();
    const auto existing_size =
        std::filesystem::exists(path, error) && !error
            ? std::filesystem::file_size(path, error)
            : 0U;
    if (error || existing_size + record.size() > kMaxNormalSessionDurableFileBytes) {
      return false;
    }

    std::ofstream out(path, std::ios::binary | std::ios::app);
    if (!out) return false;
    out.write(record.data(), static_cast<std::streamsize>(record.size()));
    out.flush();
    return out.good();
  }

  [[nodiscard]] bool compact(const NormalSessionCheckpoint& checkpoint) override {
    if (!NormalSessionJournalPolicy::valid_checkpoint(checkpoint)) return false;

    std::optional<NormalSessionCheckpoint> base_checkpoint;
    std::error_code error;
    const auto checkpoint_exists = std::filesystem::exists(checkpoint_path(), error);
    if (error) return false;
    if (checkpoint_exists) {
      base_checkpoint = read_checkpoint();
      if (!base_checkpoint.has_value()) return false;
      if (checkpoint.journal_high_water_mark < base_checkpoint->journal_high_water_mark) {
        return false;
      }
    }

    const auto journal = read_journal();
    if (!journal.has_value()) return false;

    std::vector<NormalSessionJournalEntry> covered;
    std::vector<NormalSessionJournalEntry> retained;
    covered.reserve(journal->size());
    retained.reserve(journal->size());
    for (const auto& entry : *journal) {
      if (entry.sequence <= checkpoint.journal_high_water_mark) {
        covered.push_back(entry);
      } else {
        retained.push_back(entry);
      }
    }

    const auto replayed =
        NormalSessionJournalPolicy::replay(base_checkpoint, std::move(covered));
    if (!replayed.accepted ||
        replayed.journal_high_water_mark != checkpoint.journal_high_water_mark ||
        !same_projection(checkpoint, replayed)) {
      return false;
    }

    // Write the checkpoint first. If the process dies before journal trimming,
    // replay ignores the stale <= high-water records and remains deterministic.
    if (!write_checkpoint_file(checkpoint)) return false;

    std::string journal_bytes;
    for (const auto& entry : retained) {
      journal_bytes += serialize_journal_record(entry);
      if (journal_bytes.size() > kMaxNormalSessionDurableFileBytes) return false;
    }
    return atomic_replace(journal_path(), journal_bytes);
  }

  [[nodiscard]] NormalSessionReplayResult replay_from_disk() const override {
    std::optional<NormalSessionCheckpoint> checkpoint;
    std::error_code error;
    const auto checkpoint_exists = std::filesystem::exists(checkpoint_path(), error);
    if (error) return rejected("checkpoint existence check failed");
    if (checkpoint_exists) {
      checkpoint = read_checkpoint();
      if (!checkpoint.has_value()) return rejected("checkpoint is corrupt or unsupported");
    }

    const auto journal = read_journal();
    if (!journal.has_value()) return rejected("journal is corrupt or unsupported");
    return NormalSessionJournalPolicy::replay(std::move(checkpoint), *journal);
  }

  [[nodiscard]] std::optional<NormalSessionCheckpoint> read_checkpoint() const {
    const auto bytes = read_bounded(checkpoint_path());
    if (!bytes.has_value()) return std::nullopt;
    return parse_checkpoint(*bytes);
  }

  [[nodiscard]] std::optional<std::vector<NormalSessionJournalEntry>> read_journal() const {
    std::error_code error;
    if (!std::filesystem::exists(journal_path(), error)) {
      if (error) return std::nullopt;
      return std::vector<NormalSessionJournalEntry>{};
    }

    const auto bytes = read_bounded(journal_path());
    if (!bytes.has_value()) return std::nullopt;

    std::vector<NormalSessionJournalEntry> entries;
    std::istringstream input(*bytes);
    std::string line;
    while (std::getline(input, line)) {
      if (line.empty()) continue;
      const auto parsed = parse_journal_record(line);
      if (!parsed.has_value()) return std::nullopt;
      entries.push_back(*parsed);
      if (entries.size() > kMaxNormalSessionJournalEntries) return std::nullopt;
    }

    // A non-empty file without a final newline is a potentially interrupted
    // append. Reject rather than treating a partial tail as committed.
    if (!bytes->empty() && bytes->back() != '\n') return std::nullopt;
    return entries;
  }

  [[nodiscard]] bool erase_all() {
    std::error_code error;
    bool ok = true;
    for (const auto& path : {checkpoint_path(), journal_path()}) {
      if (!std::filesystem::exists(path, error)) {
        if (error) return false;
        continue;
      }
      if (!std::filesystem::remove(path, error) || error) ok = false;
      error.clear();
    }
    return ok;
  }

  [[nodiscard]] const std::filesystem::path& directory() const noexcept {
    return directory_;
  }

 private:
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
      const NormalSessionCheckpoint& checkpoint,
      const NormalSessionReplayResult& replayed) {
    if (checkpoint.journal_id != replayed.journal_id ||
        checkpoint.profile_id != replayed.profile_id ||
        checkpoint.privacy_context_id != replayed.privacy_context_id ||
        checkpoint.session_epoch != replayed.session_epoch ||
        checkpoint.journal_high_water_mark != replayed.journal_high_water_mark ||
        checkpoint.lifecycle_state != replayed.lifecycle_state ||
        checkpoint.retired_window_ids != replayed.retired_window_ids ||
        checkpoint.retired_tab_ids != replayed.retired_tab_ids ||
        checkpoint.windows.size() != replayed.windows.size()) {
      return false;
    }
    for (std::size_t index = 0; index < checkpoint.windows.size(); ++index) {
      if (!same_window(checkpoint.windows[index], replayed.windows[index])) return false;
    }
    return true;
  }

  [[nodiscard]] bool write_checkpoint_file(
      const NormalSessionCheckpoint& checkpoint) const {
    const auto payload = serialize_checkpoint(checkpoint);
    if (payload.size() > kMaxNormalSessionDurableFileBytes) return false;
    return atomic_replace(checkpoint_path(), payload);
  }

  [[nodiscard]] static NormalSessionReplayResult rejected(std::string error) {
    NormalSessionReplayResult result;
    result.error = std::move(error);
    return result;
  }

  [[nodiscard]] std::filesystem::path journal_path() const {
    return directory_ / "normal-session.journal";
  }

  [[nodiscard]] std::filesystem::path checkpoint_path() const {
    return directory_ / "normal-session.checkpoint";
  }

  [[nodiscard]] static std::optional<std::string> read_bounded(
      const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size > kMaxNormalSessionDurableFileBytes) return std::nullopt;

    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream out;
    out << in.rdbuf();
    if (!in.good() && !in.eof()) return std::nullopt;
    return out.str();
  }

  [[nodiscard]] bool atomic_replace(
      const std::filesystem::path& final_path,
      std::string_view bytes) const {
    std::error_code error;
    std::filesystem::create_directories(directory_, error);
    if (error) return false;

    auto temp_path = final_path;
    temp_path += ".tmp";

    {
      std::ofstream out(temp_path, std::ios::binary | std::ios::trunc);
      if (!out) return false;
      out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
      out.flush();
      if (!out.good()) {
        std::filesystem::remove(temp_path, error);
        return false;
      }
    }

    std::filesystem::rename(temp_path, final_path, error);
    if (!error) return true;

    std::filesystem::remove(final_path, error);
    error.clear();
    std::filesystem::rename(temp_path, final_path, error);
    if (error) {
      std::filesystem::remove(temp_path, error);
      return false;
    }
    return true;
  }

  [[nodiscard]] static std::string escape(std::string_view value) {
    std::ostringstream out;
    for (const unsigned char ch : value) {
      if (ch == '\\' || ch == '\t' || ch == '\n' || ch == '\r') {
        out << '\\' << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<unsigned int>(ch) << std::dec;
      } else {
        out << static_cast<char>(ch);
      }
    }
    return out.str();
  }

  [[nodiscard]] static std::optional<std::string> unescape(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index) {
      if (value[index] != '\\') {
        out.push_back(value[index]);
        continue;
      }
      if (index + 2 >= value.size()) return std::nullopt;
      const auto high = hex_nibble(value[index + 1]);
      const auto low = hex_nibble(value[index + 2]);
      if (high < 0 || low < 0) return std::nullopt;
      out.push_back(static_cast<char>((high << 4) | low));
      index += 2;
    }
    return out;
  }

  [[nodiscard]] static int hex_nibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
  }

  [[nodiscard]] static std::vector<std::string_view> split_tabs(std::string_view line) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (start <= line.size()) {
      const auto position = line.find('\t', start);
      fields.push_back(line.substr(
          start,
          position == std::string_view::npos ? line.size() - start : position - start));
      if (position == std::string_view::npos) break;
      start = position + 1;
    }
    return fields;
  }

  template <typename UInt>
  [[nodiscard]] static std::optional<UInt> parse_unsigned(std::string_view text) {
    UInt value{};
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) return std::nullopt;
    return value;
  }

  [[nodiscard]] static std::uint64_t fnv1a64(std::string_view bytes) {
    // Deterministic accidental-corruption detection only. This is not a MAC.
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char byte : bytes) {
      hash ^= byte;
      hash *= 1099511628211ULL;
    }
    return hash;
  }

  [[nodiscard]] static std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
  }

  [[nodiscard]] static std::string serialize_journal_payload(
      const NormalSessionJournalEntry& entry) {
    std::ostringstream out;
    out << "J\t"
        << entry.schema_version << '\t'
        << entry.sequence << '\t'
        << entry.created_unix_ms << '\t'
        << static_cast<unsigned int>(entry.operation) << '\t'
        << static_cast<unsigned int>(entry.lifecycle_state) << '\t';
    if (entry.position.has_value()) {
      out << *entry.position;
    } else {
      out << '-';
    }
    out << '\t'
        << escape(entry.journal_id) << '\t'
        << escape(entry.profile_id) << '\t'
        << escape(entry.privacy_context_id) << '\t'
        << escape(entry.session_epoch) << '\t'
        << escape(entry.window_id) << '\t'
        << escape(entry.tab_id) << '\t'
        << escape(entry.url) << '\t'
        << escape(entry.title);
    return out.str();
  }

  [[nodiscard]] static std::string serialize_journal_record(
      const NormalSessionJournalEntry& entry) {
    const auto payload = serialize_journal_payload(entry);
    return payload + '\t' + hex64(fnv1a64(payload)) + '\n';
  }

  [[nodiscard]] static std::optional<NormalSessionJournalEntry> parse_journal_record(
      std::string_view line) {
    const auto checksum_separator = line.rfind('\t');
    if (checksum_separator == std::string_view::npos) return std::nullopt;
    const auto payload = line.substr(0, checksum_separator);
    const auto supplied_checksum = line.substr(checksum_separator + 1);
    if (supplied_checksum != hex64(fnv1a64(payload))) return std::nullopt;

    const auto fields = split_tabs(payload);
    if (fields.size() != 15 || fields[0] != "J") return std::nullopt;

    const auto schema = parse_unsigned<std::uint64_t>(fields[1]);
    const auto sequence = parse_unsigned<std::uint64_t>(fields[2]);
    const auto created = parse_unsigned<std::uint64_t>(fields[3]);
    const auto operation = parse_unsigned<unsigned int>(fields[4]);
    const auto lifecycle = parse_unsigned<unsigned int>(fields[5]);
    if (!schema || !sequence || !created || !operation || !lifecycle ||
        *operation > static_cast<unsigned int>(NormalSessionJournalOperation::reorder_tab) ||
        *lifecycle > static_cast<unsigned int>(NormalSessionLifecycleState::abnormal_termination)) {
      return std::nullopt;
    }

    std::optional<std::size_t> position;
    if (fields[6] != "-") {
      const auto parsed_position = parse_unsigned<std::uint64_t>(fields[6]);
      if (!parsed_position ||
          *parsed_position > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return std::nullopt;
      }
      position = static_cast<std::size_t>(*parsed_position);
    }

    const auto journal_id = unescape(fields[7]);
    const auto profile_id = unescape(fields[8]);
    const auto privacy_context_id = unescape(fields[9]);
    const auto session_epoch = unescape(fields[10]);
    const auto window_id = unescape(fields[11]);
    const auto tab_id = unescape(fields[12]);
    const auto url = unescape(fields[13]);
    const auto title = unescape(fields[14]);
    if (!journal_id || !profile_id || !privacy_context_id || !session_epoch ||
        !window_id || !tab_id || !url || !title) {
      return std::nullopt;
    }

    NormalSessionJournalEntry entry{
        .schema_version = *schema,
        .journal_id = *journal_id,
        .profile_id = *profile_id,
        .privacy_context_id = *privacy_context_id,
        .session_epoch = *session_epoch,
        .sequence = *sequence,
        .created_unix_ms = *created,
        .operation = static_cast<NormalSessionJournalOperation>(*operation),
        .lifecycle_state = static_cast<NormalSessionLifecycleState>(*lifecycle),
        .window_id = *window_id,
        .tab_id = *tab_id,
        .position = position,
        .url = *url,
        .title = *title,
    };
    if (!NormalSessionJournalPolicy::valid_journal_entry(entry)) return std::nullopt;
    return entry;
  }

  [[nodiscard]] static std::string serialize_checkpoint(
      const NormalSessionCheckpoint& checkpoint) {
    std::ostringstream out;
    out << "GCNS1\n";
    out << "C\t"
        << checkpoint.schema_version << '\t'
        << checkpoint.created_unix_ms << '\t'
        << checkpoint.journal_high_water_mark << '\t'
        << static_cast<unsigned int>(checkpoint.lifecycle_state) << '\t'
        << escape(checkpoint.checkpoint_id) << '\t'
        << escape(checkpoint.journal_id) << '\t'
        << escape(checkpoint.profile_id) << '\t'
        << escape(checkpoint.privacy_context_id) << '\t'
        << escape(checkpoint.session_epoch) << '\t'
        << checkpoint.integrity_checksum << '\n';

    for (const auto& window : checkpoint.windows) {
      out << "W\t"
          << escape(window.window_id) << '\t'
          << static_cast<unsigned int>(window.privacy_mode) << '\t'
          << escape(window.active_tab_id.value_or("")) << '\n';
      for (const auto& tab : window.tabs) {
        out << "T\t"
            << escape(tab.tab_id) << '\t'
            << escape(tab.url) << '\t'
            << escape(tab.title) << '\t'
            << escape(tab.workspace_id) << '\t'
            << escape(tab.group_id.value_or("")) << '\t'
            << escape(tab.split_id.value_or("")) << '\t'
            << (tab.pinned ? 1 : 0) << '\t'
            << (tab.active ? 1 : 0) << '\t'
            << tab.last_active_unix_ms << '\n';
      }
      out << "E\n";
    }

    for (const auto& id : checkpoint.retired_window_ids) {
      out << "RW\t" << escape(id) << '\n';
    }
    for (const auto& id : checkpoint.retired_tab_ids) {
      out << "RT\t" << escape(id) << '\n';
    }
    out << "Z\n";
    return out.str();
  }

  [[nodiscard]] static std::optional<NormalSessionCheckpoint> parse_checkpoint(
      std::string_view bytes) {
    if (bytes.size() > kMaxNormalSessionDurableFileBytes) return std::nullopt;

    std::istringstream input(std::string{bytes});
    std::string line;
    if (!std::getline(input, line) || line != "GCNS1") return std::nullopt;

    NormalSessionCheckpoint checkpoint;
    RecoverableWindow* current_window = nullptr;
    bool saw_checkpoint = false;
    bool saw_end = false;

    while (std::getline(input, line)) {
      if (line.empty()) continue;
      const auto fields = split_tabs(line);
      if (fields.empty()) return std::nullopt;

      if (fields[0] == "C") {
        if (saw_checkpoint || current_window != nullptr || fields.size() != 11) {
          return std::nullopt;
        }
        const auto schema = parse_unsigned<std::uint64_t>(fields[1]);
        const auto created = parse_unsigned<std::uint64_t>(fields[2]);
        const auto high_water = parse_unsigned<std::uint64_t>(fields[3]);
        const auto lifecycle = parse_unsigned<unsigned int>(fields[4]);
        const auto checkpoint_id = unescape(fields[5]);
        const auto journal_id = unescape(fields[6]);
        const auto profile_id = unescape(fields[7]);
        const auto privacy_context = unescape(fields[8]);
        const auto session_epoch = unescape(fields[9]);
        if (!schema || !created || !high_water || !lifecycle ||
            *lifecycle > static_cast<unsigned int>(NormalSessionLifecycleState::abnormal_termination) ||
            !checkpoint_id || !journal_id || !profile_id || !privacy_context ||
            !session_epoch) {
          return std::nullopt;
        }
        checkpoint.schema_version = *schema;
        checkpoint.created_unix_ms = *created;
        checkpoint.journal_high_water_mark = *high_water;
        checkpoint.lifecycle_state = static_cast<NormalSessionLifecycleState>(*lifecycle);
        checkpoint.checkpoint_id = *checkpoint_id;
        checkpoint.journal_id = *journal_id;
        checkpoint.profile_id = *profile_id;
        checkpoint.privacy_context_id = *privacy_context;
        checkpoint.session_epoch = *session_epoch;
        checkpoint.integrity_checksum = std::string{fields[10]};
        saw_checkpoint = true;
        continue;
      }

      if (!saw_checkpoint || saw_end) return std::nullopt;

      if (fields[0] == "W") {
        if (current_window != nullptr || fields.size() != 4) return std::nullopt;
        const auto window_id = unescape(fields[1]);
        const auto privacy = parse_unsigned<unsigned int>(fields[2]);
        const auto active = unescape(fields[3]);
        if (!window_id || !privacy || !active ||
            *privacy > static_cast<unsigned int>(SessionPrivacyMode::isolated_private)) {
          return std::nullopt;
        }
        RecoverableWindow window;
        window.window_id = *window_id;
        window.privacy_mode = static_cast<SessionPrivacyMode>(*privacy);
        if (!active->empty()) window.active_tab_id = *active;
        checkpoint.windows.push_back(std::move(window));
        current_window = &checkpoint.windows.back();
      } else if (fields[0] == "T") {
        if (current_window == nullptr || fields.size() != 10) return std::nullopt;
        const auto tab_id = unescape(fields[1]);
        const auto url = unescape(fields[2]);
        const auto title = unescape(fields[3]);
        const auto workspace = unescape(fields[4]);
        const auto group = unescape(fields[5]);
        const auto split = unescape(fields[6]);
        const auto pinned = parse_unsigned<unsigned int>(fields[7]);
        const auto active = parse_unsigned<unsigned int>(fields[8]);
        const auto last_active = parse_unsigned<std::uint64_t>(fields[9]);
        if (!tab_id || !url || !title || !workspace || !group || !split ||
            !pinned || !active || !last_active || *pinned > 1 || *active > 1) {
          return std::nullopt;
        }
        RecoverableTab tab;
        tab.tab_id = *tab_id;
        tab.url = *url;
        tab.title = *title;
        tab.workspace_id = *workspace;
        tab.pinned = *pinned == 1;
        tab.active = *active == 1;
        tab.last_active_unix_ms = *last_active;
        if (!group->empty()) tab.group_id = *group;
        if (!split->empty()) tab.split_id = *split;
        current_window->tabs.push_back(std::move(tab));
      } else if (fields[0] == "E") {
        if (fields.size() != 1 || current_window == nullptr) return std::nullopt;
        current_window = nullptr;
      } else if (fields[0] == "RW") {
        if (fields.size() != 2 || current_window != nullptr) return std::nullopt;
        const auto id = unescape(fields[1]);
        if (!id) return std::nullopt;
        checkpoint.retired_window_ids.push_back(*id);
      } else if (fields[0] == "RT") {
        if (fields.size() != 2 || current_window != nullptr) return std::nullopt;
        const auto id = unescape(fields[1]);
        if (!id) return std::nullopt;
        checkpoint.retired_tab_ids.push_back(*id);
      } else if (fields[0] == "Z") {
        if (fields.size() != 1 || current_window != nullptr) return std::nullopt;
        saw_end = true;
      } else {
        return std::nullopt;
      }
    }

    if (!saw_checkpoint || !saw_end || current_window != nullptr) return std::nullopt;
    if (!NormalSessionJournalPolicy::valid_checkpoint(checkpoint)) return std::nullopt;
    return checkpoint;
  }

  std::filesystem::path directory_;
};

}  // namespace goreecloud::browser
