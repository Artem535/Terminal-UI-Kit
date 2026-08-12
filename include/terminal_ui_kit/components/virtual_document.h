#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/core/text_style.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

class StreamingDocument;

struct VirtualDocumentOptions {
  StreamingDocument* document = nullptr;
  Theme theme = default_dark_theme();
  bool follow = true;
  int tab_width = 8;
  bool show_line_numbers = false;
  // Width of the line-number gutter in columns. Line numbers longer than this
  // are rendered in full without truncation and without any padding; shorter
  // numbers are right-aligned within the gutter. A value of 0 is handled
  // safely (numbers render unpadded).
  std::size_t gutter_width = 5;
  std::function<void(std::string)> on_copy;
};

class VirtualDocumentImpl;

class VirtualDocument {
 public:
  explicit VirtualDocument(VirtualDocumentOptions options);
  ~VirtualDocument() = default;

  ftxui::Component component() const;

  bool follow() const;
  void set_follow(bool follow);
  void scroll_to_bottom();
  void scroll_to_line(std::size_t logical_line_index);

 private:
  std::shared_ptr<VirtualDocumentImpl> impl_;
};

}  // namespace terminal_ui_kit