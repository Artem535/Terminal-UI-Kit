#include "terminal_ui_kit/components/progress_tree.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/progress_bar.h"
#include "terminal_ui_kit/components/spinner.h"
#include "terminal_ui_kit/components/status_indicator.h"
#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {
namespace {

void append_rows(const std::vector<ProgressTask>& tasks, const Theme& theme, std::size_t depth,
                 const std::unordered_set<std::string>& collapsed, const std::string& selected_id,
                 const ftxui::Component& spinner, ftxui::Elements& rows) {
  for (const ProgressTask& task : tasks) {
    const bool has_children = !task.children.empty();
    const char* chevron = collapsed.contains(task.id) ? "▸" : "▾";
    ftxui::Elements row = {
        ftxui::text(std::string(depth * 2, ' ')),
        ftxui::text(has_children ? chevron : " ") | to_decorator(theme.muted),
        ftxui::text(" "),
        StatusIndicator(task.status, task.label, theme),
    };
    if (task.fraction) {
      ProgressBarOptions options;
      options.width = 8;
      row.push_back(ftxui::text(" "));
      row.push_back(ProgressBar(*task.fraction, theme, options));
    } else if (task.status == Status::kRunning) {
      row.push_back(ftxui::text(" "));
      row.push_back(spinner->Render());
    }
    row.push_back(ftxui::filler());
    if (!task.detail.empty()) {
      row.push_back(ftxui::text(task.detail) | to_decorator(theme.muted));
    }
    ftxui::Element element = ftxui::hbox(std::move(row));
    if (task.id == selected_id) {
      element = element | ftxui::inverted;
    }
    rows.push_back(std::move(element));
    if (!collapsed.contains(task.id)) {
      append_rows(task.children, theme, depth + 1, collapsed, selected_id, spinner, rows);
    }
  }
}

void append_visible(const std::vector<ProgressTask>& tasks,
                    const std::unordered_set<std::string>& collapsed,
                    std::vector<const ProgressTask*>& visible) {
  for (const ProgressTask& task : tasks) {
    visible.push_back(&task);
    if (!collapsed.contains(task.id)) {
      append_visible(task.children, collapsed, visible);
    }
  }
}

}  // namespace

class ProgressTreeImpl : public ftxui::ComponentBase {
 public:
  ProgressTreeImpl(std::vector<ProgressTask> tasks, const Theme& theme)
      : tasks_(std::move(tasks)), theme_(theme), spinner_(Spinner()) {
    Add(spinner_);
    NormalizeSelection();
  }

  void SetTasks(std::vector<ProgressTask> tasks) {
    tasks_ = std::move(tasks);
    NormalizeSelection();
  }

 private:
  ftxui::Element Render() override {
    ftxui::Elements rows;
    append_rows(tasks_, theme_, 0, collapsed_, selected_id_, spinner_, rows);
    return ftxui::vbox(std::move(rows));
  }
  bool Focusable() const override { return true; }
  bool OnEvent(ftxui::Event event) override {
    std::vector<const ProgressTask*> visible;
    append_visible(tasks_, collapsed_, visible);
    if (visible.empty()) {
      return false;
    }

    const auto selected = std::find_if(visible.begin(), visible.end(), [this](const auto* task) {
      return task->id == selected_id_;
    });
    if (selected == visible.end()) {
      selected_id_ = visible.front()->id;
      return false;
    }

    const std::size_t selected_index = static_cast<std::size_t>(selected - visible.begin());
    if (event == ftxui::Event::ArrowDown && selected_index + 1 < visible.size()) {
      selected_id_ = visible[selected_index + 1]->id;
      return true;
    }
    if (event == ftxui::Event::ArrowUp && selected_index > 0) {
      selected_id_ = visible[selected_index - 1]->id;
      return true;
    }
    if (event == ftxui::Event::Home) {
      selected_id_ = visible.front()->id;
      return true;
    }
    if (event == ftxui::Event::End) {
      selected_id_ = visible.back()->id;
      return true;
    }
    if ((event == ftxui::Event::Character(' ') || event == ftxui::Event::Return) &&
        !(*selected)->children.empty()) {
      const std::string& id = (*selected)->id;
      if (!collapsed_.erase(id)) {
        collapsed_.insert(id);
      }
      return true;
    }
    return false;
  }

  void NormalizeSelection() {
    std::vector<const ProgressTask*> visible;
    append_visible(tasks_, collapsed_, visible);
    if (visible.empty()) {
      selected_id_.clear();
      return;
    }
    const auto selected = std::find_if(visible.begin(), visible.end(), [this](const auto* task) {
      return task->id == selected_id_;
    });
    if (selected == visible.end()) {
      selected_id_ = visible.front()->id;
    }
  }

  std::vector<ProgressTask> tasks_;
  Theme theme_;
  ftxui::Component spinner_;
  std::unordered_set<std::string> collapsed_;
  std::string selected_id_;
};

ftxui::Component ProgressTree(std::vector<ProgressTask> tasks, const Theme& theme) {
  return ftxui::Make<ProgressTreeImpl>(std::move(tasks), theme);
}

ftxui::Component TaskList(std::vector<ProgressTask> tasks, const Theme& theme) {
  return ProgressTree(std::move(tasks), theme);
}

ProgressTreeModel::ProgressTreeModel(const Theme& theme)
    : impl_(ftxui::Make<ProgressTreeImpl>(std::vector<ProgressTask>{}, theme)) {}

void ProgressTreeModel::set_tasks(std::vector<ProgressTask> tasks) {
  impl_->SetTasks(std::move(tasks));
}

ftxui::Component ProgressTreeModel::component() const { return impl_; }

}  // namespace terminal_ui_kit
