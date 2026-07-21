#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct ProgressTask {
  std::string id;
  std::string label;
  Status status = Status::kIdle;
  std::optional<double> fraction;
  std::string detail;
  std::vector<ProgressTask> children;
};

class ProgressTreeImpl;

class ProgressTreeModel {
 public:
  explicit ProgressTreeModel(const Theme& theme);
  void set_tasks(std::vector<ProgressTask> tasks);
  ftxui::Component component() const;

 private:
  std::shared_ptr<ProgressTreeImpl> impl_;
};

ftxui::Component ProgressTree(std::vector<ProgressTask> tasks, const Theme& theme);
ftxui::Component TaskList(std::vector<ProgressTask> tasks, const Theme& theme);

}  // namespace terminal_ui_kit
