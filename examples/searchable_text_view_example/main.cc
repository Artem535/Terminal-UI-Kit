// Example: SearchableTextView
//
// Demonstrates the reusable, searchable text view: literal and regular
// expression search, case sensitivity, match navigation with wrap-around,
// UTF-8 text, and auto-scrolling to the active match. The sample document is
// generated deterministically (no randomness, no network).
//
// Controls:
//   /            open search
//   <type>       edit the query (incremental, live results)
//   Enter        apply the query / close the search prompt
//   n            next match
//   N            previous match
//   c            toggle case sensitivity
//   r            toggle regex mode
//   Esc          close/cancel search
//   q            quit

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/searchable_text_view.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

// Deterministic sample data: a sizeable document with repeated tokens and a
// few UTF-8 (Cyrillic) lines so matches and byte offsets are meaningful.
std::vector<std::string> BuildDocument() {
  const char* sentences[] = {
      "The quick brown router jumps over the lazy dog.",
      "A router forwards packets between networks.",
      "Packet routing is the fundamental task of a router.",
      "Every home has at least one router these days.",
      "The needle is hidden in a haystack of routers.",
      "Find the needle among all the routers on this line.",
      "Router configuration requires care and attention.",
      "Some routers support wireless and some do not.",
      "The needle you seek is the very first one here.",
      "Routing tables tell each router where to go next.",
      "Do not confuse a byte offset with a cell column.",
  };
  const char* cyrillic[] = {
      "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82 \xD0\xBC\xD0\xB8\xD1\x80!",
      "\xD0\x9C\xD0\xB0\xD1\x80\xD1\x88\xD1\x80\xD1\x83\xD1\x82\xD0\xB8\xD0\xB7\xD0\xB0\xD1\x86\xD0"
      "\xB8\xD1\x8F "
      "\xD1\x81\xD0\xB5\xD1\x82\xD0\xB8.",
  };

  std::vector<std::string> lines;
  lines.reserve(500);
  // Repeat the base set several times so n/N wrapping is obvious.
  for (int round = 0; round < 40; ++round) {
    const std::size_t count = sizeof(sentences) / sizeof(sentences[0]);
    for (std::size_t i = 0; i < count; ++i) {
      lines.push_back(std::string("L") + std::to_string(round) + "." + std::to_string(i) + " " +
                      sentences[i]);
    }
  }
  for (const char* c : cyrillic) {
    lines.push_back(std::string(c));
  }
  return lines;
}

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  using namespace ftxui;

  const Theme& theme = default_dark_theme();

  SearchableTextViewOptions opts;
  opts.theme = theme;
  SearchableTextView view(std::move(opts));
  view.set_lines(BuildDocument());

  auto screen = ScreenInteractive::Fullscreen();

  auto footer_element = [&] {
    std::vector<Element> row;
    if (view.search_open()) {
      row.push_back(text("Search: " + view.query()) | bold);
    } else {
      switch (view.status()) {
        case SearchStatus::kMatches: {
          const std::size_t current = view.current_match_index().value_or(0) + 1;
          row.push_back(text("Match " + std::to_string(current) + "/" +
                             std::to_string(view.match_count()) + "  " + view.query()));
          break;
        }
        case SearchStatus::kNoResults:
          row.push_back(text("No results: " + view.query()) | ftxui::color(ftxui::Color::Yellow));
          break;
        case SearchStatus::kInvalidRegex:
          row.push_back(text("Invalid regex: " + view.query()) | ftxui::bold |
                        ftxui::color(ftxui::Color::Red));
          break;
        default:
          row.push_back(text("No query"));
          break;
      }
    }
    row.push_back(text(view.case_sensitive() ? "  Case: exact" : "  Case: any") | dim);
    row.push_back(text(view.use_regex() ? "  Mode: regex" : "  Mode: literal") | dim);
    return hbox(std::move(row));
  };

  auto help_element = [] {
    return hbox({
        text("[/] open  ") | dim,
        text("[n/N] next/prev  ") | dim,
        text("[c] case  ") | dim,
        text("[r] regex  ") | dim,
        text("[Esc] cancel  ") | dim,
        text("[q] quit  ") | dim,
    });
  };

  // Single root Renderer whose child is the searchable view, so the view
  // receives all keyboard events regardless of where focus currently sits.
  // `q` quits only while the search prompt is closed: while typing, `q` is a
  // query character and must reach the search input.
  auto root = Renderer(view.component(), [&] {
    return vbox({
        text("Terminal UI Kit - Searchable Text View") | bold,
        separator(),
        view.component()->Render() | flex,
        separator(),
        footer_element(),
        separator(),
        help_element(),
    });
  });

  root |= CatchEvent([&](const Event& event) {
    if (!view.search_open() && event == Event::Character('q')) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
  return 0;
}