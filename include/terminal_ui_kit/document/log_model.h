#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {

enum class LogSeverity { kTrace, kDebug, kInfo, kWarning, kError };

struct LogEntry {
  std::string timestamp;
  LogSeverity severity = LogSeverity::kInfo;
  StyledText message;
};

struct LogFilter {
  std::optional<LogSeverity> minimum_severity;
  std::string substring;
};

class LogModel {
 public:
  explicit LogModel(std::size_t retention_limit = 0);

  void append(LogEntry entry);
  void clear();
  void set_filter(LogFilter filter);

  [[nodiscard]] std::size_t size() const;
  const LogEntry& at(std::size_t index) const;
  [[nodiscard]] std::uint64_t revision() const { return revision_; }

 private:
  void rebuild_visible_indexes();

  std::deque<LogEntry> entries_;
  std::vector<std::size_t> visible_indexes_;
  LogFilter filter_;
  std::size_t retention_limit_;
  std::uint64_t revision_ = 0;
};

}  // namespace terminal_ui_kit
