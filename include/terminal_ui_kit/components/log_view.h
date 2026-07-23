#pragma once

#include <memory>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/components/virtual_list.h"
#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

struct LogViewOptions {
  StreamingDocument* document = nullptr;
  LogModel* log_model = nullptr;
  bool follow = true;
  bool show_timestamp = true;
  bool show_severity = true;
  Theme theme = default_dark_theme();
};

class LogViewImpl;

class LogView {
 public:
  explicit LogView(LogViewOptions options);

  ftxui::Component component() const;

  bool follow() const;
  void set_follow(bool follow);
  void scroll_to_bottom();

 private:
  std::shared_ptr<LogViewImpl> impl_;
};

}  // namespace terminal_ui_kit