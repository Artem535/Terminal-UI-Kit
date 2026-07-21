#pragma once

#include <string>

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Renders an icon + text pair for one of the eight semantic statuses (PRD
// section 38). The icon carries meaning independently of color so the
// indicator remains legible with without_color() applied; kRunning also
// gets a blink cue -- a fully animated frame-by-frame spinner is Spinner's
// job, composed separately by callers that want one.
ftxui::Element StatusIndicator(Status status, std::string text, const Theme& theme);

}  // namespace terminal_ui_kit
