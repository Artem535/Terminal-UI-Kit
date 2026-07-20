#pragma once

#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "terminal_ui_kit/core/text_style.h"

namespace terminal_ui_kit {

struct Hyperlink {
  std::string url;

  friend bool operator==(const Hyperlink&, const Hyperlink&) = default;
};

// A run of text sharing one style (PRD section 15.1).
struct TextSpan {
  std::string text;
  TextStyle style;
  std::optional<Hyperlink> hyperlink;
};

// An ordered sequence of styled spans forming one logical piece of text
// (PRD section 15.1), consumed by Markdown, code views, diff views, log
// views, transcripts, and prompt previews (section 15.3).
class StyledText {
 public:
  void append(TextSpan span) { spans_.push_back(std::move(span)); }

  [[nodiscard]] std::span<const TextSpan> spans() const { return spans_; }

 private:
  std::vector<TextSpan> spans_;
};

}  // namespace terminal_ui_kit
