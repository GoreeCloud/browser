#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "goreecloud/browser/advanced_tab_manager.hpp"
#include "goreecloud/browser/engine.hpp"
#include "goreecloud/browser/internal_pages.hpp"
#include "goreecloud/browser/tab.hpp"

namespace goreecloud::browser {

class WindowController {
 public:
  explicit WindowController(EngineContext& context,
                            bool private_window = false,
                            AdvancedTabManager* tab_manager = nullptr,
                            std::string window_id = "window-1")
      : context_(context),
        private_window_(private_window),
        tab_manager_(tab_manager),
        window_id_(std::move(window_id)) {}

  [[nodiscard]] Tab& new_tab(std::string initial_url = std::string{kNewTabUrl}) {
    EngineViewOptions options;
    options.initial_url = std::move(initial_url);

    auto view = context_.create_view(options);
    const auto id = std::string{"tab-"} + std::to_string(next_tab_id_++);
    tabs_.push_back(std::make_unique<Tab>(id, std::move(view)));
    activate(tabs_.size() - 1);
    return *tabs_.back();
  }

  [[nodiscard]] bool close_tab(std::string_view id, bool explicit_protected_close = false) {
    if (tab_manager_ && !tab_manager_->close_tabs({std::string{id}}, explicit_protected_close)) {
      return false;
    }

    const auto it = std::find_if(tabs_.begin(), tabs_.end(), [id](const auto& tab) {
      return tab->id() == id;
    });
    if (it == tabs_.end()) return false;

    const auto removed_index = static_cast<std::size_t>(std::distance(tabs_.begin(), it));
    (*it)->set_lifecycle_state(TabLifecycleState::Closed);
    tabs_.erase(it);
    selected_tab_ids_.erase(
        std::remove(selected_tab_ids_.begin(), selected_tab_ids_.end(), id),
        selected_tab_ids_.end());

    if (tabs_.empty()) {
      active_index_.reset();
      return true;
    }
    if (!active_index_.has_value() || removed_index <= *active_index_) {
      activate(std::min(removed_index, tabs_.size() - 1));
    }
    return true;
  }

  [[nodiscard]] bool activate_tab(std::string_view id) {
    for (std::size_t index = 0; index < tabs_.size(); ++index) {
      if (tabs_[index]->id() == id) {
        activate(index);
        return true;
      }
    }
    return false;
  }

  [[nodiscard]] bool select_tab(std::string_view id, bool additive) {
    if (!contains_tab(id)) return false;
    if (!additive) selected_tab_ids_.clear();
    if (std::find(selected_tab_ids_.begin(), selected_tab_ids_.end(), id) == selected_tab_ids_.end()) {
      selected_tab_ids_.push_back(std::string{id});
    }
    return true;
  }

  void clear_selection() { selected_tab_ids_.clear(); }
  [[nodiscard]] const std::vector<std::string>& selected_tab_ids() const noexcept { return selected_tab_ids_; }

  [[nodiscard]] Tab* active_tab() noexcept {
    if (!active_index_.has_value() || *active_index_ >= tabs_.size()) return nullptr;
    return tabs_[*active_index_].get();
  }
  [[nodiscard]] const Tab* active_tab() const noexcept {
    if (!active_index_.has_value() || *active_index_ >= tabs_.size()) return nullptr;
    return tabs_[*active_index_].get();
  }

  [[nodiscard]] std::size_t tab_count() const noexcept { return tabs_.size(); }
  [[nodiscard]] bool private_window() const noexcept { return private_window_; }
  [[nodiscard]] const std::string& window_id() const noexcept { return window_id_; }

  [[nodiscard]] std::vector<std::string> tab_ids() const {
    std::vector<std::string> ids;
    ids.reserve(tabs_.size());
    for (const auto& tab : tabs_) ids.push_back(tab->id());
    return ids;
  }

  [[nodiscard]] std::vector<const Tab*> tab_views() const {
    std::vector<const Tab*> views;
    views.reserve(tabs_.size());
    for (const auto& tab : tabs_) views.push_back(tab.get());
    return views;
  }

  bool sleep_selected_tabs() {
    if (!tab_manager_ || selected_tab_ids_.empty()) return false;
    const bool result = tab_manager_->sleep_tabs(selected_tab_ids_);
    if (result) {
      for (const auto& id : selected_tab_ids_) set_lifecycle(id, TabLifecycleState::Suspended);
    }
    return result;
  }

  bool wake_selected_tabs() {
    if (!tab_manager_ || selected_tab_ids_.empty()) return false;
    const bool result = tab_manager_->wake_tabs(selected_tab_ids_);
    if (result) {
      for (const auto& id : selected_tab_ids_) {
        set_lifecycle(id, active_tab() && active_tab()->id() == id
                              ? TabLifecycleState::Active
                              : TabLifecycleState::Background);
      }
    }
    return result;
  }

  bool protect_selected_tabs(bool protect) {
    return tab_manager_ && !selected_tab_ids_.empty() &&
           tab_manager_->protect_tabs(selected_tab_ids_, protect);
  }

  bool lock_selected_tabs(bool lock) {
    return tab_manager_ && !selected_tab_ids_.empty() &&
           tab_manager_->lock_tabs(selected_tab_ids_, lock);
  }

  bool pin_selected_tabs(bool pin) {
    return tab_manager_ && !selected_tab_ids_.empty() &&
           tab_manager_->pin_tabs(selected_tab_ids_, pin);
  }

  void navigate_active(std::string_view url) {
    if (auto* tab = active_tab()) tab->engine_view().navigate(url);
  }

  void open_home() { navigate_active(kHomeUrl); }
  void open_settings() { navigate_active(kSettingsUrl); }

  void back() { if (auto* tab = active_tab()) tab->engine_view().go_back(); }
  void forward() { if (auto* tab = active_tab()) tab->engine_view().go_forward(); }

  void reload_or_stop() {
    if (auto* tab = active_tab()) {
      if (tab->engine_view().navigation_state().loading) tab->engine_view().stop();
      else tab->engine_view().reload();
    }
  }

 private:
  [[nodiscard]] bool contains_tab(std::string_view id) const {
    return std::any_of(tabs_.begin(), tabs_.end(), [id](const auto& tab) { return tab->id() == id; });
  }

  void set_lifecycle(std::string_view id, TabLifecycleState state) {
    for (auto& tab : tabs_) {
      if (tab->id() == id) {
        tab->set_lifecycle_state(state);
        return;
      }
    }
  }

  void activate(std::size_t index) {
    for (std::size_t i = 0; i < tabs_.size(); ++i) {
      tabs_[i]->set_lifecycle_state(i == index ? TabLifecycleState::Active
                                              : TabLifecycleState::Background);
    }
    active_index_ = index;
    selected_tab_ids_.clear();
    selected_tab_ids_.push_back(tabs_[index]->id());
  }

  EngineContext& context_;
  bool private_window_{false};
  AdvancedTabManager* tab_manager_{nullptr};
  std::string window_id_;
  std::vector<std::unique_ptr<Tab>> tabs_;
  std::vector<std::string> selected_tab_ids_;
  std::optional<std::size_t> active_index_;
  std::uint64_t next_tab_id_{1};
};

}  // namespace goreecloud::browser
