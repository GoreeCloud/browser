#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace goreecloud::browser {

enum class SplitLayout { none, two_vertical, two_horizontal, three_primary_left, three_primary_top, four_quadrant };
enum class TabProtection { normal, locked, protected_tab };
enum class TabSleepPolicy { automatic, never_sleep_tab, never_sleep_site };

struct TabResourceState {
  std::uint64_t approximate_memory_bytes{0};
  double cpu_percent{0.0};
  std::uint64_t network_bytes_per_second{0};
  bool audio_active{false};
  bool media_active{false};
  bool download_active{false};
  bool sleeping{false};
};

struct ManagedTabState {
  std::string tab_id;
  std::string window_id;
  std::string workspace_id;
  std::optional<std::string> group_id;
  std::optional<std::string> split_id;
  bool pinned{false};
  TabProtection protection{TabProtection::normal};
  TabSleepPolicy sleep_policy{TabSleepPolicy::automatic};
  TabResourceState resources;
};

struct TabGroupState {
  std::string id;
  std::string name;
  std::string color_token;
  std::string icon_token;
  bool collapsed{false};
  std::vector<std::string> tab_ids;
};

struct SplitViewState {
  std::string id;
  SplitLayout layout{SplitLayout::none};
  std::vector<std::string> tab_ids;
  std::vector<double> pane_fractions;
  std::optional<std::size_t> expanded_pane;
};

struct WorkspaceState {
  std::string id;
  std::string name;
  std::vector<std::string> tab_ids;
  std::vector<TabGroupState> groups;
  std::vector<SplitViewState> splits;
};

struct TabSearchQuery {
  std::string text;
  std::optional<std::string> workspace_id;
  std::optional<std::string> group_id;
  bool sleeping_only{false};
  bool pinned_only{false};
  bool protected_only{false};
  bool audio_only{false};
  bool split_only{false};
};

struct ClosedSessionItem {
  enum class Kind { tab, window, group, workspace };
  Kind kind{Kind::tab};
  std::string id;
  std::string recovery_payload_id;
};

class AdvancedTabManager {
 public:
  virtual ~AdvancedTabManager() = default;

  virtual bool register_tab(ManagedTabState state) = 0;
  [[nodiscard]] virtual std::optional<ManagedTabState> tab_state(
      std::string_view tab_id) const = 0;

  virtual bool duplicate_tabs(const std::vector<std::string>& tab_ids, bool new_window) = 0;
  virtual bool move_tabs(const std::vector<std::string>& tab_ids, const std::string& window_id) = 0;
  [[nodiscard]] virtual bool can_close_tabs(
      const std::vector<std::string>& tab_ids,
      bool explicit_protected_close) const = 0;
  virtual bool close_tabs(const std::vector<std::string>& tab_ids, bool explicit_protected_close) = 0;
  virtual bool reload_tabs(const std::vector<std::string>& tab_ids) = 0;
  virtual bool pin_tabs(const std::vector<std::string>& tab_ids, bool pinned) = 0;
  virtual bool lock_tabs(const std::vector<std::string>& tab_ids, bool locked) = 0;
  virtual bool protect_tabs(const std::vector<std::string>& tab_ids, bool protected_tab) = 0;
  virtual bool sleep_tabs(const std::vector<std::string>& tab_ids) = 0;
  virtual bool wake_tabs(const std::vector<std::string>& tab_ids) = 0;
  virtual bool mute_tabs(const std::vector<std::string>& tab_ids, bool muted) = 0;

  virtual std::string create_group(const std::string& workspace_id, const std::string& name) = 0;
  virtual bool assign_tabs_to_group(const std::vector<std::string>& tab_ids, const std::string& group_id) = 0;
  virtual std::string create_workspace(const std::string& name) = 0;
  virtual bool move_tabs_to_workspace(const std::vector<std::string>& tab_ids, const std::string& workspace_id) = 0;
  virtual std::string create_split(SplitLayout layout, const std::vector<std::string>& tab_ids) = 0;
  virtual bool exit_split(const std::string& split_id, bool keep_tabs_open) = 0;

  [[nodiscard]] virtual std::vector<ManagedTabState> search(const TabSearchQuery& query) const = 0;
  [[nodiscard]] virtual std::vector<std::vector<std::string>> duplicate_url_sets() const = 0;
  [[nodiscard]] virtual std::vector<ClosedSessionItem> recently_closed() const = 0;
  virtual bool restore(const ClosedSessionItem& item) = 0;
};

}  // namespace goreecloud::browser
