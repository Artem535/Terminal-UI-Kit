#include "terminal_ui_kit/components/log_view.h"

#include <string>

#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>
#include <gtest/gtest.h>

#include "terminal_ui_kit/core/styled_text.h"
#include "terminal_ui_kit/core/text_style.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/testing/virtual_screen.h"

namespace terminal_ui_kit {
namespace {

TEST(LogView, RawModeShowsStreamingLines) {
  StreamingDocument doc;
  doc.append("hello\nworld");
  doc.finish();

  LogViewOptions opts;
  opts.document = &doc;
  LogView view(opts);

  test_support::render_to_screen(view.component()->Render(), 20, 3);
  ftxui::Screen screen = test_support::render_to_screen(view.component()->Render(), 20, 3);
  std::string text = screen.ToString();
  EXPECT_NE(text.find("hello"), std::string::npos);
  EXPECT_NE(text.find("world"), std::string::npos);
}

TEST(LogView, StructuredModeShowsLogEntry) {
  LogModel model;
  LogEntry entry;
  entry.timestamp = "12:00";
  entry.severity = LogSeverity::kInfo;
  TextStyle style;
  style.bold = true;
  entry.message.append(TextSpan{"test message", style, std::nullopt});
  model.append(std::move(entry));

  LogViewOptions opts;
  opts.log_model = &model;
  LogView view(opts);

  test_support::render_to_screen(view.component()->Render(), 30, 3);
  std::string text = test_support::render_to_text(view.component()->Render(), 30, 3);
  EXPECT_NE(text.find("test message"), std::string::npos);
}

TEST(LogView, ArrowUpDisablesFollow) {
  StreamingDocument doc;
  doc.append("line1\nline2\nline3\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = true;
  LogView view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
}

TEST(LogView, SetFollowReenablesFollowing) {
  StreamingDocument doc;
  doc.append("line1\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = true;
  LogView view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());

  view.component()->OnEvent(ftxui::Event::End);
  EXPECT_TRUE(view.follow());
}

TEST(LogView, FollowAutoScrollsOnNewData) {
  StreamingDocument doc;
  doc.append("line1\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = true;
  LogView view(opts);

  view.component()->Render();

  doc.append("line2\n");

  std::string text = test_support::render_to_text(view.component()->Render(), 20, 2);
  EXPECT_NE(text.find("line2"), std::string::npos);
}

TEST(LogView, ArrowUpWhileNotFollowedLeavesFollowOff) {
  StreamingDocument doc;
  doc.append("line1\n");

  LogViewOptions opts;
  opts.document = &doc;
  opts.follow = false;
  LogView view(opts);

  view.component()->OnEvent(ftxui::Event::ArrowUp);
  EXPECT_FALSE(view.follow());
}

}  // namespace
}  // namespace terminal_ui_kit