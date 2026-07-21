#pragma once

namespace terminal_ui_kit {

// The set of semantic run/task statuses shared by StatusIndicator and
// CollapsiblePanel (PRD section 38.1).
enum class Status {
  kIdle,
  kPending,
  kRunning,
  kSuccess,
  kWarning,
  kError,
  kCancelled,
  kPaused,
};

}  // namespace terminal_ui_kit
