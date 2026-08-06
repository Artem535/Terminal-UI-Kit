#include "terminal_ui_kit/components/toast_view.h"

#include <chrono>
#include <memory>
#include <string>

#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/toast_manager.h"
#include "terminal_ui_kit/testing/virtual_screen.h"
#include "terminal_ui_kit/theme/theme.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using std::chrono_literals::operator""s;

std::string StripAnsi(const std::string& in) {
  std::string out;
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size();) {
    if (in[i] == '\x1b' && i + 1 < in.size() && in[i + 1] == '[') {
      i += 2;
      while (i < in.size() && !(in[i] >= 0x40 && in[i] <= 0x7e)) {
        ++i;
      }
      if (i < in.size()) {
        ++i;  // consume the CSI final byte
      }
      continue;
    }
    out.push_back(in[i]);
    ++i;
  }
  return out;
}

std::string Render(ToastManager& manager, const Theme& theme, int width = 60, int height = 6) {
  return test_support::render_to_text(ToastElement(manager, theme), width, height);
}

class ToastViewTest : public ::testing::Test {
 protected:
  ToastManager manager_{std::make_shared<SystemToastClock>(), 5};
};

TEST_F(ToastViewTest, EachSeverityRendersItsTag) {
  struct Case {
    ToastSeverity severity;
    const char* tag;
  };
  const Case cases[] = {
      {ToastSeverity::kInfo, "INFO"},
      {ToastSeverity::kSuccess, "OK"},
      {ToastSeverity::kWarning, "WARN"},
      {ToastSeverity::kError, "ERR"},
  };
  for (const Case& c : cases) {
    ToastManager m(std::make_shared<SystemToastClock>(), 5);
    m.Show(ToastOptions{"msg", c.severity, 5s, std::nullopt});
    const std::string text = StripAnsi(Render(m, default_dark_theme()));
    EXPECT_NE(text.find(c.tag), std::string::npos) << "severity tag missing: " << c.tag;
  }
}

TEST_F(ToastViewTest, LongMessageWrapsWithoutLosingWords) {
  const std::string message = "one two three four five six seven eight nine ten eleven twelve";
  manager_.Show(ToastOptions{message, ToastSeverity::kInfo, 5s, std::nullopt});

  const std::string text =
      StripAnsi(Render(manager_, default_dark_theme(), /*width=*/24, /*height=*/10));
  for (const char* word : {"one", "four", "eight", "twelve"}) {
    EXPECT_NE(text.find(word), std::string::npos) << "word lost after wrapping: " << word;
  }
}

TEST_F(ToastViewTest, EmptyMessageStillRendersSeverity) {
  manager_.Show(ToastOptions{"", ToastSeverity::kError, 5s, std::nullopt});

  const std::string text = StripAnsi(Render(manager_, default_dark_theme()));
  EXPECT_NE(text.find("ERR"), std::string::npos);
}

TEST_F(ToastViewTest, ActionLabelIsRendered) {
  manager_.Show(ToastOptions{"hello", ToastSeverity::kInfo, 5s, ToastAction{"Undo", [] {}}});

  const std::string text = StripAnsi(Render(manager_, default_dark_theme()));
  EXPECT_NE(text.find("Undo"), std::string::npos);
}

TEST_F(ToastViewTest, TimedToastShowsCountdown) {
  manager_.Show(ToastOptions{"hello", ToastSeverity::kInfo, 5s, std::nullopt});

  const std::string text = StripAnsi(Render(manager_, default_dark_theme()));
  EXPECT_NE(text.find("5s"), std::string::npos);
}

TEST_F(ToastViewTest, FocusMarkerIsRendered) {
  manager_.Show(ToastOptions{"a", ToastSeverity::kInfo, 5s, std::nullopt});
  manager_.Show(ToastOptions{"b", ToastSeverity::kInfo, 5s, std::nullopt});
  manager_.SetFocus(1);

  const std::string text = StripAnsi(Render(manager_, default_dark_theme()));
  EXPECT_NE(text.find("\u258C"), std::string::npos);  // ▌
}

bool HasForegroundColor(const ftxui::Screen& screen, const ftxui::Color& color) {
  for (int y = 0; y < screen.dimy(); ++y) {
    for (int x = 0; x < screen.dimx(); ++x) {
      if (screen.PixelAt(x, y).foreground_color == color) {
        return true;
      }
    }
  }
  return false;
}

TEST_F(ToastViewTest, NoColorRemovesForeground) {
  // A vivid error color so the pixel comparison is unambiguous.
  Theme theme = default_dark_theme();
  theme.error = TextStyle{/*bold=*/false,
                          /*italic=*/false,
                          /*underline=*/false,
                          /*dim=*/false,
                          /*strikethrough=*/false,
                          /*foreground=*/Color{240, 32, 32},
                          /*background=*/std::nullopt};
  manager_.Show(ToastOptions{"hello", ToastSeverity::kError, 5s, std::nullopt});

  const ftxui::Screen colored =
      test_support::render_to_screen(ToastElement(manager_, theme), 40, 5);
  const ftxui::Screen plain =
      test_support::render_to_screen(ToastElement(manager_, without_color(theme)), 40, 5);

  const ftxui::Color vivid = ftxui::Color::RGB(240, 32, 32);
  EXPECT_TRUE(HasForegroundColor(colored, vivid))
      << "colored render should paint the error severity with the theme color";
  EXPECT_FALSE(HasForegroundColor(plain, vivid))
      << "no-color render must not paint any pixel with the theme color";
}

TEST_F(ToastViewTest, NoColorKeepsSeverityAndFocusLegible) {
  manager_.Show(ToastOptions{"a", ToastSeverity::kWarning, 5s, std::nullopt});
  manager_.Show(ToastOptions{"b", ToastSeverity::kSuccess, 5s, std::nullopt});
  manager_.SetFocus(1);

  const std::string text = StripAnsi(test_support::render_to_text(
      ToastElement(manager_, without_color(default_dark_theme())), 60, 6));
  EXPECT_NE(text.find("WARN"), std::string::npos);
  EXPECT_NE(text.find("OK"), std::string::npos);
  EXPECT_NE(text.find("\u258C"), std::string::npos);  // focus glyph stays visible
}

TEST_F(ToastViewTest, NarrowTerminalWrapsMessage) {
  manager_.Show(ToastOptions{"narrow terminal toast", ToastSeverity::kInfo, 5s, std::nullopt});

  // A narrow but usable width: the fixed prefix + countdown leave a handful of
  // columns for the message, which must wrap rather than be lost.
  const std::string text =
      StripAnsi(Render(manager_, default_dark_theme(), /*width=*/24, /*height=*/10));
  for (const char* word : {"narrow", "terminal", "toast"}) {
    EXPECT_NE(text.find(word), std::string::npos) << "word lost at narrow width: " << word;
  }
}

}  // namespace
}  // namespace terminal_ui_kit