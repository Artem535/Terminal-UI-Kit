#include "terminal_ui_kit/components/collapsible_panel.h"

#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

ftxui::Component MakeBody(const std::string& text) {
  return ftxui::Renderer([text] { return ftxui::text(text); });
}

TEST(CollapsiblePanel, CollapsedByDefaultHidesBody) {
  CollapsiblePanelOptions options;
  options.title = "Tool call";

  ftxui::Component panel =
      CollapsiblePanel(options, MakeBody("body content"), default_dark_theme());
  std::string text = test_support::render_to_text(panel->Render(), 40, 3);

  EXPECT_NE(text.find("Tool call"), std::string::npos);
  EXPECT_EQ(text.find("body content"), std::string::npos);
}

TEST(CollapsiblePanel, SpaceKeyExpandsAndRevealsBody) {
  CollapsiblePanelOptions options;
  options.title = "Tool call";

  ftxui::Component panel =
      CollapsiblePanel(options, MakeBody("body content"), default_dark_theme());
  panel->OnEvent(ftxui::Event::Character(' '));

  std::string text = test_support::render_to_text(panel->Render(), 40, 3);
  EXPECT_NE(text.find("body content"), std::string::npos);
}

TEST(CollapsiblePanel, SpaceKeyTwiceCollapsesAgain) {
  CollapsiblePanelOptions options;
  options.title = "Tool call";

  ftxui::Component panel =
      CollapsiblePanel(options, MakeBody("body content"), default_dark_theme());
  panel->OnEvent(ftxui::Event::Character(' '));
  panel->OnEvent(ftxui::Event::Character(' '));

  std::string text = test_support::render_to_text(panel->Render(), 40, 3);
  EXPECT_EQ(text.find("body content"), std::string::npos);
}

TEST(CollapsiblePanel, InitiallyExpandedShowsBodyImmediately) {
  CollapsiblePanelOptions options;
  options.title = "Tool call";
  options.initially_expanded = true;

  ftxui::Component panel =
      CollapsiblePanel(options, MakeBody("body content"), default_dark_theme());
  std::string text = test_support::render_to_text(panel->Render(), 40, 3);

  EXPECT_NE(text.find("body content"), std::string::npos);
}

TEST(CollapsiblePanel, RendersSummaryAndDuration) {
  CollapsiblePanelOptions options;
  options.title = "Tool call";
  options.summary = "Ran 3 tests";
  options.duration_text = "1.2s";

  ftxui::Component panel =
      CollapsiblePanel(options, MakeBody("body content"), default_dark_theme());
  std::string text = test_support::render_to_text(panel->Render(), 60, 3);

  EXPECT_NE(text.find("Ran 3 tests"), std::string::npos);
  EXPECT_NE(text.find("1.2s"), std::string::npos);
}

TEST(CollapsiblePanel, HeaderIsFocusable) {
  CollapsiblePanelOptions options;
  options.title = "Tool call";

  ftxui::Component panel =
      CollapsiblePanel(options, MakeBody("body content"), default_dark_theme());
  EXPECT_TRUE(panel->Focusable());
}

}  // namespace
}  // namespace terminal_ui_kit
