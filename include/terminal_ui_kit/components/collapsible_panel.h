#pragma once

#include <optional>
#include <string>

#include <ftxui/component/component.hpp>

#include "terminal_ui_kit/components/status.h"
#include "terminal_ui_kit/theme/theme.h"

namespace terminal_ui_kit {

// Header content for a CollapsiblePanel: a title, optional status icon,
// optional one-line summary, optional right-aligned duration/action slot, and
// the panel's initial expanded state.
struct CollapsiblePanelOptions {
  std::string title;
  std::optional<Status> status;
  std::string summary;
  std::string duration_text;
  bool initially_expanded = false;
};

// A toggleable panel. Its body is rendered and receives events only while
// expanded; Space, Enter, or a header click toggles its expanded state.
ftxui::Component CollapsiblePanel(CollapsiblePanelOptions options, ftxui::Component body,
                                  const Theme& theme);

}  // namespace terminal_ui_kit
