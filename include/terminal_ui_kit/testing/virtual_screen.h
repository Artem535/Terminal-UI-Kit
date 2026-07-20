#pragma once

#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

namespace terminal_ui_kit::test_support {

// Renders `element` into an off-screen ftxui::Screen of the given size, for
// rendering golden tests that don't need a real terminal (PRD section
// 49.2). Named `test_support` rather than `testing` to avoid colliding with
// GoogleTest's own `::testing` namespace.
inline ftxui::Screen render_to_screen(const ftxui::Element& element, int width, int height) {
  ftxui::Screen screen(width, height);
  ftxui::Render(screen, element);
  return screen;
}

inline std::string render_to_text(const ftxui::Element& element, int width, int height) {
  return render_to_screen(element, width, height).ToString();
}

}  // namespace terminal_ui_kit::test_support
