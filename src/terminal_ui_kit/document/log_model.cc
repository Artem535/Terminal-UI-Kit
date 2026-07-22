#include "terminal_ui_kit/document/log_model.h"

#include <stdexcept>
#include <utility>

namespace terminal_ui_kit {
namespace {

std::string plain_text(const StyledText& message) {
  std::string result;
  for (const TextSpan& span : message.spans()) {
    result += span.text;
  }
  return result;
}

bool meets_minimum(LogSeverity severity, const std::optional<LogSeverity>& minimum) {
  return !minimum || static_cast<int>(severity) >= static_cast<int>(*minimum);
}

}  // namespace

LogModel::LogModel(std::size_t retention_limit) : retention_limit_(retention_limit) {
  rebuild_visible_indexes();
}

void LogModel::append(LogEntry entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  entries_.push_back(std::move(entry));
  if (retention_limit_ != 0 && entries_.size() > retention_limit_) {
    entries_.pop_front();
  }
  rebuild_visible_indexes();
  ++revision_;
}

void LogModel::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  entries_.clear();
  visible_indexes_.clear();
  ++revision_;
}

void LogModel::set_filter(LogFilter filter) {
  std::lock_guard<std::mutex> lock(mutex_);
  filter_ = std::move(filter);
  rebuild_visible_indexes();
  ++revision_;
}

std::size_t LogModel::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return visible_indexes_.size();
}

const LogEntry& LogModel::at(std::size_t index) const {
  std::lock_guard<std::mutex> lock(mutex_);
  if (index >= visible_indexes_.size()) {
    throw std::out_of_range("LogModel index out of range");
  }
  return entries_[visible_indexes_[index]];
}

std::uint64_t LogModel::revision() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return revision_;
}

void LogModel::rebuild_visible_indexes() {
  visible_indexes_.clear();
  for (std::size_t index = 0; index < entries_.size(); ++index) {
    const LogEntry& entry = entries_[index];
    if (!meets_minimum(entry.severity, filter_.minimum_severity)) {
      continue;
    }
    if (!filter_.substring.empty() &&
        plain_text(entry.message).find(filter_.substring) == std::string::npos) {
      continue;
    }
    visible_indexes_.push_back(index);
  }
}

}  // namespace terminal_ui_kit
