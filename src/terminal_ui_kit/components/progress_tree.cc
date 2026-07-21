#include "terminal_ui_kit/components/progress_tree.h"

#include <unordered_set>
#include <utility>

#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/progress_bar.h"
#include "terminal_ui_kit/components/spinner.h"
#include "terminal_ui_kit/components/status_indicator.h"

namespace terminal_ui_kit {
namespace {

void append_rows(const std::vector<ProgressTask>& tasks, const Theme& theme, std::size_t depth,
                 const std::unordered_set<std::string>& collapsed, std::size_t& row_index,
                 std::size_t selected, ftxui::Elements& rows) {
  for (const ProgressTask& task : tasks) {
    ftxui::Elements row = {ftxui::text(std::string(depth * 2, ' ')),
                           StatusIndicator(task.status, task.label, theme)};
    if (task.fraction) {
      ProgressBarOptions options;
      options.width = 8;
      row.push_back(ftxui::text(" "));
      row.push_back(ProgressBar(*task.fraction, theme, options));
    } else if (task.status == Status::kRunning) {
      row.push_back(ftxui::text(" "));
      row.push_back(Spinner()->Render());
    }
    ftxui::Element element = ftxui::hbox(std::move(row));
    if (row_index == selected) {
      element = element | ftxui::focus;
    }
    rows.push_back(std::move(element));
    ++row_index;
    if (!collapsed.contains(task.id)) {
      append_rows(task.children, theme, depth + 1, collapsed, row_index, selected, rows);
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
      : tasks_(std::move(tasks)), theme_(theme) {}

  void SetTasks(std::vector<ProgressTask> tasks) { tasks_ = std::move(tasks); }

 private:
  ftxui::Element Render() override {
    ftxui::Elements rows;
    std::size_t row_index = 0;
    append_rows(tasks_, theme_, 0, collapsed_, row_index, selected_, rows);
    return ftxui::vbox(std::move(rows));
  }
  bool Focusable() const override { return true; }
  bool OnEvent(ftxui::Event event) override {
    std::vector<const ProgressTask*> visible;
    append_visible(tasks_, collapsed_, visible);
    if (event == ftxui::Event::ArrowDown && selected_ + 1 < visible.size()) {
      ++selected_;
      return true;
    }
    if (event == ftxui::Event::ArrowUp && selected_ > 0) {
      --selected_;
      return true;
    }
    if (event == ftxui::Event::Home) {
      selected_ = 0;
      return true;
    }
    if (event == ftxui::Event::End && !visible.empty()) {
      selected_ = visible.size() - 1;
      return true;
    }
    if ((event == ftxui::Event::Character(' ') || event == ftxui::Event::Return) &&
        !visible.empty() && !visible[selected_]->children.empty()) {
      const std::string& id = visible[selected_]->id;
      if (!collapsed_.erase(id)) {
        collapsed_.insert(id);
      }
      return true;
    }
    return false;
  }
  std::vector<ProgressTask> tasks_;
  Theme theme_;
  std::unordered_set<std::string> collapsed_;
  std::size_t selected_ = 0;
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
