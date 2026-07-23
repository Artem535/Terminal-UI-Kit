#pragma once

#include <functional>
#include <memory>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

class MarkdownDocument;

struct MarkdownViewOptions {
  Theme theme = default_dark_theme();
  std::function<void(std::string url)> on_link;
};

ftxui::Component MarkdownView(
    std::shared_ptr<MarkdownDocument> document,
    MarkdownViewOptions options);

}  // namespace terminal_ui_kit