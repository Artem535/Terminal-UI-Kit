#include "terminal_ui_kit/components/toast.h"

#include <chrono>
#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using Duration = std::chrono::steady_clock::duration;

std::string StripAnsi(std::string input) {
  std::string out;
  out.reserve(input.size());
  std::size_t i = 0;
  while (i < input.size()) {
    if (input[i] == '\x1b') {
      // Consume CSI sequence up to the terminating letter.
      if (i + 1 < input.size() && input[i + 1] == '[') {
        i += 2;
        while (i < input.size() && input[i] != 'm' && input[i] < 0x40) {
          ++i;
        }
        if (i < input.size() && input[i] == 'm') {
          ++i;
        }
      } else if (i + 1 < input.size() && input[i + 1] == ']') {
        // OSC (e.g. title) — skip until BEL or ST; not expected here.
        i += 2;
        while (i < input.size() && input[i] != '\a') {
          ++i;
        }
        if (i < input.size()) {
          ++i;
        }
      } else {
        ++i;
      }
      continue;
    }
    out.push_back(input[i]);
    ++i;
  }
  return out;
}

ToastOptions Timed(std::string message, std::chrono::milliseconds duration,
                   ToastSeverity severity = ToastSeverity::kInfo) {
  ToastOptions options;
  options.message = std::move(message);
  options.severity = severity;
  options.duration = duration;
  return options;
}

ftxui::Component MakeView(ToastManager& manager, ToastViewOptions options = {}) {
  return ftxui::Make<ToastView>(manager, default_dark_theme(), options);
}

TEST(ToastView, RendersSingleToastMessage) {
  ToastManager manager;
  manager.show(Timed("operation finished", std::chrono::seconds(5)));

  auto view = MakeView(manager);
  std::string text = StripAnsi(test_support::render_to_text(view->Render(), 60, 5));
  EXPECT_NE(text.find("operation finished"), std::string::npos);
}

TEST(ToastView, RendersSeverityIcons) {
  ToastManager manager;
  manager.show(Timed("info", std::chrono::seconds(5), ToastSeverity::kInfo));
  manager.show(Timed("ok", std::chrono::seconds(5), ToastSeverity::kSuccess));
  manager.show(Timed("warn", std::chrono::seconds(5), ToastSeverity::kWarning));
  manager.show(Timed("err", std::chrono::seconds(5), ToastSeverity::kError));

  auto view = MakeView(manager);
  std::string text = StripAnsi(test_support::render_to_text(view->Render(), 60, 8));
  EXPECT_NE(text.find("✓"), std::string::npos);
  EXPECT_NE(text.find("▲"), std::string::npos);
  EXPECT_NE(text.find("✗"), std::string::npos);
  EXPECT_NE(text.find("ℹ"), std::string::npos);
}

TEST(ToastView, RendersActionLabel) {
  ToastManager manager;
  ToastOptions options = Timed("save", std::chrono::seconds(5));
  options.action = ToastAction{"undo", [] {}};
  manager.show(options);

  auto view = MakeView(manager);
  std::string text = StripAnsi(test_support::render_to_text(view->Render(), 60, 5));
  EXPECT_NE(text.find("undo"), std::string::npos);
}

TEST(ToastView, FocusedToastRendersMarker) {
  ToastManager manager;
  std::size_t first = manager.show(Timed("first", std::chrono::seconds(5)));
  manager.show(Timed("second", std::chrono::seconds(5)));

  auto view = MakeView(manager);
  // Nothing focused yet: no marker.
  std::string unfocused = StripAnsi(test_support::render_to_text(view->Render(), 60, 5));
  EXPECT_EQ(unfocused.find("▶"), std::string::npos);

  manager.set_focused(first);
  std::string focused = StripAnsi(test_support::render_to_text(view->Render(), 60, 5));
  EXPECT_NE(focused.find("▶"), std::string::npos);
}

TEST(ToastView, NoColorModeOmitsColorCodes) {
  ToastManager manager;
  manager.show(Timed("error", std::chrono::seconds(5), ToastSeverity::kError));

  ToastViewOptions options;
  options.no_color = true;
  auto view = MakeView(manager, options);
  std::string text = test_support::render_to_text(view->Render(), 60, 5);

  // Color is emitted as ESC[38;2;... / ESC[48;2;...
  EXPECT_EQ(text.find("38;2"), std::string::npos);
  EXPECT_EQ(text.find("48;2"), std::string::npos);
  // Severity icon must still be present without color.
  std::string plain = StripAnsi(text);
  EXPECT_NE(plain.find("✗"), std::string::npos);
}

TEST(ToastView, NarrowTerminalRendersWithoutCrash) {
  ToastManager manager;
  manager.show(Timed("a fairly long message that must still be visible when the terminal is narrow",
                     std::chrono::seconds(5)));

  auto view = MakeView(manager);
  std::string text = StripAnsi(test_support::render_to_text(view->Render(), 12, 8));
  // The message wraps rather than being dropped.
  EXPECT_NE(text.find("message"), std::string::npos);
}

TEST(ToastView, TabMovesFocus) {
  ToastManager manager;
  manager.show(Timed("a", std::chrono::seconds(5)));
  manager.show(Timed("b", std::chrono::seconds(5)));

  auto view = MakeView(manager);
  EXPECT_FALSE(manager.has_focus());
  EXPECT_TRUE(view->OnEvent(ftxui::Event::Tab));
  EXPECT_TRUE(manager.has_focus());
  EXPECT_TRUE(view->OnEvent(ftxui::Event::TabReverse));
}

TEST(ToastView, EnterInvokesFocusedAction) {
  ToastManager manager;
  int calls = 0;
  ToastOptions options = Timed("act", std::chrono::seconds(5));
  options.action = ToastAction{"run", [&calls] { ++calls; }};
  std::size_t id = manager.show(options);

  auto view = MakeView(manager);
  manager.set_focused(id);
  EXPECT_TRUE(view->OnEvent(ftxui::Event::Return));
  EXPECT_EQ(calls, 1);
  EXPECT_TRUE(manager.empty());
}

TEST(ToastView, DeleteClosesFocusedToast) {
  ToastManager manager;
  std::size_t id = manager.show(Timed("go", std::chrono::seconds(5)));

  auto view = MakeView(manager);
  manager.set_focused(id);
  EXPECT_TRUE(view->OnEvent(ftxui::Event::Delete));
  EXPECT_TRUE(manager.empty());
}

}  // namespace
}  // namespace terminal_ui_kit
