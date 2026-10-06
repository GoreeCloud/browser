#pragma once

#include "goreecloud/browser/normal_session_journal.hpp"

namespace goreecloud::browser {

class NormalSessionDurableStore {
 public:
  virtual ~NormalSessionDurableStore() = default;

  [[nodiscard]] virtual bool append(
      const NormalSessionJournalEntry& entry) = 0;
  [[nodiscard]] virtual bool compact(
      const NormalSessionCheckpoint& checkpoint) = 0;
  [[nodiscard]] virtual NormalSessionReplayResult replay_from_disk() const = 0;
  [[nodiscard]] virtual bool erase_all() = 0;
};

}  // namespace goreecloud::browser
