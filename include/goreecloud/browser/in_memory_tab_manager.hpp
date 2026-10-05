#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "goreecloud/browser/advanced_tab_manager.hpp"

namespace goreecloud::browser {

class InMemoryAdvancedTabManager final : public AdvancedTabManager {
 public:
  bool register_tab(ManagedTabState state) override {
    if (state.tab_id.empty()) return false;
    tabs_[state.tab_id] = std::move(state);
    return true;
  }

  [[nodiscard]] std::optional<ManagedTabState> tab_state(
      std::string_view tab_id) const override {
    const auto found = tabs_.find(std::string{tab_id});
    if (found == tabs_.end()) return std::nullopt;
    return found->second;
  }

  bool duplicate_tabs(const std::vector<std::string>& tab_ids, bool) override {
    return all_exist(tab_ids);
  }

  bool move_tabs(const std::vector<std::string>& tab_ids,
                 const std::string& window_id) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) tabs_[id].window_id = window_id;
    return true;
  }

  bool close_tabs(const std::vector<std::string>& tab_ids,
                  bool explicit_protected_close) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) {
      if (tabs_[id].protection == TabProtection::protected_tab &&
          !explicit_protected_close) {
        return false;
      }
    }
    for (const auto& id : tab_ids) {
      closed_.push_back({ClosedSessionItem::Kind::tab, id, "memory:" + id});
      tabs_.erase(id);
    }
    return true;
  }

  bool reload_tabs(const std::vector<std::string>& tab_ids) override {
    return all_exist(tab_ids);
  }

  bool pin_tabs(const std::vector<std::string>& tab_ids, bool pinned) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) tabs_[id].pinned = pinned;
    return true;
  }

  bool lock_tabs(const std::vector<std::string>& tab_ids, bool locked) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) {
      tabs_[id].protection = locked ? TabProtection::locked : TabProtection::normal;
    }
    return true;
  }

  bool protect_tabs(const std::vector<std::string>& tab_ids,
                    bool protected_tab) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) {
      tabs_[id].protection = protected_tab ? TabProtection::protected_tab
                                           : TabProtection::normal;
    }
    return true;
  }

  bool sleep_tabs(const std::vector<std::string>& tab_ids) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) {
      auto& tab = tabs_[id];
      if (tab.sleep_policy != TabSleepPolicy::never_sleep_tab &&
          !tab.resources.audio_active && !tab.resources.media_active &&
          !tab.resources.download_active) {
        tab.resources.sleeping = true;
      }
    }
    return true;
  }

  bool wake_tabs(const std::vector<std::string>& tab_ids) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) tabs_[id].resources.sleeping = false;
    return true;
  }

  bool mute_tabs(const std::vector<std::string>& tab_ids, bool muted) override {
    if (!all_exist(tab_ids)) return false;
    for (const auto& id : tab_ids) {
      if (muted) tabs_[id].resources.audio_active = false;
    }
    return true;
  }

  std::string create_group(const std::string& workspace_id,
                           const std::string& name) override {
    const auto id = "group-" + std::to_string(next_group_id_++);
    groups_[id] = TabGroupState{id, name, "default", "tabs", false, {}};
    group_workspace_[id] = workspace_id;
    return id;
  }

  bool assign_tabs_to_group(const std::vector<std::string>& tab_ids,
                            const std::string& group_id) override {
    if (!all_exist(tab_ids) || !groups_.contains(group_id)) return false;
    auto& group = groups_[group_id];
    for (const auto& id : tab_ids) {
      tabs_[id].group_id = group_id;
      if (std::find(group.tab_ids.begin(), group.tab_ids.end(), id) == group.tab_ids.end()) {
        group.tab_ids.push_back(id);
      }
    }
    return true;
  }

  std::string create_workspace(const std::string& name) override {
    const auto id = "workspace-" + std::to_string(next_workspace_id_++);
    workspaces_[id] = WorkspaceState{id, name, {}, {}, {}};
    return id;
  }

  bool move_tabs_to_workspace(const std::vector<std::string>& tab_ids,
                              const std::string& workspace_id) override {
    if (!all_exist(tab_ids) || !workspaces_.contains(workspace_id)) return false;
    for (const auto& id : tab_ids) {
      tabs_[id].workspace_id = workspace_id;
      workspaces_[workspace_id].tab_ids.push_back(id);
    }
    return true;
  }

  std::string create_split(SplitLayout layout,
                           const std::vector<std::string>& tab_ids) override {
    if (!all_exist(tab_ids) || tab_ids.size() < 2 || tab_ids.size() > 4) return {};
    const auto id = "split-" + std::to_string(next_split_id_++);
    const double fraction = 1.0 / static_cast<double>(tab_ids.size());
    SplitViewState split{id, layout, tab_ids,
                         std::vector<double>(tab_ids.size(), fraction), std::nullopt};
    splits_[id] = split;
    for (const auto& tab_id : tab_ids) tabs_[tab_id].split_id = id;
    return id;
  }

  bool exit_split(const std::string& split_id, bool) override {
    const auto it = splits_.find(split_id);
    if (it == splits_.end()) return false;
    for (const auto& tab_id : it->second.tab_ids) {
      if (tabs_.contains(tab_id)) tabs_[tab_id].split_id.reset();
    }
    splits_.erase(it);
    return true;
  }

  std::vector<ManagedTabState> search(const TabSearchQuery& query) const override {
    std::vector<ManagedTabState> results;
    const auto needle = lowercase(query.text);
    for (const auto& [id, tab] : tabs_) {
      if (query.workspace_id && tab.workspace_id != *query.workspace_id) continue;
      if (query.group_id && tab.group_id != query.group_id) continue;
      if (query.sleeping_only && !tab.resources.sleeping) continue;
      if (query.pinned_only && !tab.pinned) continue;
      if (query.protected_only && tab.protection != TabProtection::protected_tab) continue;
      if (query.audio_only && !tab.resources.audio_active) continue;
      if (query.split_only && !tab.split_id.has_value()) continue;
      if (!needle.empty() && lowercase(id).find(needle) == std::string::npos) continue;
      results.push_back(tab);
    }
    return results;
  }

  std::vector<std::vector<std::string>> duplicate_url_sets() const override {
    return {};
  }

  std::vector<ClosedSessionItem> recently_closed() const override { return closed_; }

  bool restore(const ClosedSessionItem&) override {
    // Actual restoration requires SessionStore payload materialization.
    return false;
  }

 private:
  bool all_exist(const std::vector<std::string>& ids) const {
    return std::all_of(ids.begin(), ids.end(), [this](const auto& id) {
      return tabs_.contains(id);
    });
  }

  static std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
      return static_cast<char>(std::tolower(ch));
    });
    return value;
  }

  std::unordered_map<std::string, ManagedTabState> tabs_;
  std::unordered_map<std::string, TabGroupState> groups_;
  std::unordered_map<std::string, std::string> group_workspace_;
  std::unordered_map<std::string, WorkspaceState> workspaces_;
  std::unordered_map<std::string, SplitViewState> splits_;
  std::vector<ClosedSessionItem> closed_;
  std::uint64_t next_group_id_{1};
  std::uint64_t next_workspace_id_{1};
  std::uint64_t next_split_id_{1};
};

}  // namespace goreecloud::browser
