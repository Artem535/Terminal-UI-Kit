#include "terminal_ui_kit/document/log_model.h"

#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

LogEntry Entry(LogSeverity severity, std::string text) {
  StyledText message;
  message.append(TextSpan{std::move(text), TextStyle{}, std::nullopt});
  return LogEntry{"now", severity, std::move(message)};
}

TEST(LogModel, AppendsInOrderAndSupportsUnlimitedRetention) {
  LogModel model;
  model.append(Entry(LogSeverity::kInfo, "one"));
  model.append(Entry(LogSeverity::kError, "two"));

  ASSERT_EQ(model.size(), 2u);
  EXPECT_EQ(model.at(0).message.spans()[0].text, "one");
  EXPECT_EQ(model.at(1).message.spans()[0].text, "two");
}

TEST(LogModel, EvictsOldestEntryFirst) {
  LogModel model(2);
  model.append(Entry(LogSeverity::kInfo, "one"));
  model.append(Entry(LogSeverity::kInfo, "two"));
  model.append(Entry(LogSeverity::kInfo, "three"));

  ASSERT_EQ(model.size(), 2u);
  EXPECT_EQ(model.at(0).message.spans()[0].text, "two");
  EXPECT_EQ(model.at(1).message.spans()[0].text, "three");
}

TEST(LogModel, ClearPreservesFilter) {
  LogModel model;
  model.set_filter(LogFilter{LogSeverity::kWarning, ""});
  model.append(Entry(LogSeverity::kInfo, "hidden"));
  model.append(Entry(LogSeverity::kError, "visible"));
  model.clear();
  model.append(Entry(LogSeverity::kInfo, "still hidden"));
  EXPECT_EQ(model.size(), 0u);
}

TEST(LogModel, FiltersByMinimumSeverityAndSubstring) {
  LogModel model;
  model.append(Entry(LogSeverity::kInfo, "Build passed"));
  model.append(Entry(LogSeverity::kError, "Build failed"));
  model.append(Entry(LogSeverity::kError, "Other"));
  model.set_filter(LogFilter{LogSeverity::kWarning, "Build"});

  ASSERT_EQ(model.size(), 1u);
  EXPECT_EQ(model.at(0).message.spans()[0].text, "Build failed");
}

TEST(LogModel, SearchesConcatenatedSpansCaseSensitively) {
  LogEntry entry;
  entry.message.append(TextSpan{"Hello ", TextStyle{}, std::nullopt});
  entry.message.append(TextSpan{"world", TextStyle{}, std::nullopt});
  LogModel model;
  model.append(std::move(entry));

  model.set_filter(LogFilter{std::nullopt, "Hello world"});
  EXPECT_EQ(model.size(), 1u);
  model.set_filter(LogFilter{std::nullopt, "hello"});
  EXPECT_EQ(model.size(), 0u);
}

TEST(LogModel, RevisionChangesOncePerMutation) {
  LogModel model;
  EXPECT_EQ(model.revision(), 0u);
  model.append(Entry(LogSeverity::kInfo, "x"));
  EXPECT_EQ(model.revision(), 1u);
  model.set_filter({});
  EXPECT_EQ(model.revision(), 2u);
  model.clear();
  EXPECT_EQ(model.revision(), 3u);
}

TEST(LogModel, EmptyAndFullyFilteredModelsHaveZeroSize) {
  LogModel model;
  EXPECT_EQ(model.size(), 0u);
  model.append(Entry(LogSeverity::kInfo, "x"));
  model.set_filter(LogFilter{LogSeverity::kError, ""});
  EXPECT_EQ(model.size(), 0u);
}

TEST(LogModel, AtThrowsForOutOfRangeIndex) {
  LogModel model;
  EXPECT_THROW(model.at(0), std::out_of_range);
}

}  // namespace
}  // namespace terminal_ui_kit
