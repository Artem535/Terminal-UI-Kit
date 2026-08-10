// TranscriptView example (Task H3).
//
// Simulates an interactive coding-agent session rendered in a virtualized,
// follow-end transcript: plain text, Markdown, code, logs, status, and diff
// blocks are emitted in a deterministic sequence while a live mutable tail
// streams in place. Demonstrates follow-end, manual scroll, search, bookmarks,
// mixed block heights, resize, and large-data (100,000-block) mode.
//
// Requires no network or external service; all sample data is generated
// locally and deterministically. Exit with q or Esc.

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/transcript.h"
#include "terminal_ui_kit/components/transcript_model.h"
#include "terminal_ui_kit/theme/theme.h"

using namespace terminal_ui_kit;

namespace {

// Deterministic set of blocks emitted at startup and periodically by the
// simulated agent turn. No randomness: index-driven selection keeps the demo
// reproducible run to run.
std::vector<TranscriptItem> BuildSeedBlocks() {
  std::vector<TranscriptItem> blocks;
  blocks.push_back(TextBlock{"User: please review the diff in src/core/text_wrap.cc."});
  blocks.push_back(MarkdownBlock{
      "## Assistant\nI analysed the wrapping logic. Here is the plan:\n"
      "- adopt word-boundary wrapping for CJK text\n- keep the hard-wrap path intact"});
  blocks.push_back(
      CodeBlock{"// text_wrap.cc\nfor (size_t i = 0; i < s.size(); ++i) {\n  if (s[i] == ' ') { "
                "wrap_at = i; }\n}\n",
                "cpp"});
  blocks.push_back(LogBlock{LogSeverity::kInfo, "parsing input... ok (42 ms)"});
  blocks.push_back(LogBlock{LogSeverity::kWarning, "long line exceeds 100 columns; wrapping"});
  blocks.push_back(LogBlock{LogSeverity::kError, "failed to apply hunk 3; skipping"});
  blocks.push_back(
      DiffBlock{"--- a/src/core/text_wrap.cc\n"
                "+++ b/src/core/text_wrap.cc\n"
                "@@ -12,4 +12,5 @@\n"
                " size_t WrapText(TextStyle style) {\n"
                "-  return wrap_plain_text(text, 80);\n"
                "+  return wrap_plain_text(text, 80, /*word_boundary=*/true);\n"
                "+  cache_invalidate();\n"});
  blocks.push_back(StatusBlock{Status::kRunning, "Applying changes..."});
  blocks.push_back(StatusBlock{Status::kSuccess, "Changes applied and tests pass"});
  blocks.push_back(CustomBlock{"tool:shell", "grep -n \"wrap_plain_text\" src/core/text_wrap.cc"});
  return blocks;
}

// Tokens streamed into the mutable tail, advancing deterministically.
const char* kTokenBank[] = {
    "The",     " virtualized", " transcript", " streams",  " tokens",   " into",        " a",
    " single", " tail",        " block",      " without",  " creating", " new",         " entries",
    " per",    " chunk.",      " The",        " viewport", " stays",    " responsive",  " even",
    " after",  " one",         " hundred",    " thousand", " blocks",   " accumulate.",
};

// Emits one completed block of a rotating kind to seed long-history mode.
TranscriptItem RotatingBlock(std::size_t i) {
  switch (i % 6) {
    case 0:
      return TextBlock{"note " + std::to_string(i)};
    case 1:
      return MarkdownBlock{"## section " + std::to_string(i)};
    case 2:
      return CodeBlock{"int value = " + std::to_string(i) + ";\n", "cpp"};
    case 3:
      return LogBlock{LogSeverity::kDebug, "step " + std::to_string(i)};
    case 4:
      return StatusBlock{Status::kSuccess, "step " + std::to_string(i) + " ok"};
    default:
      return CustomBlock{"item", std::to_string(i)};
  }
}

}  // namespace

int main() {
  const Theme& theme = default_dark_theme();
  TranscriptModel model;

  // A status line the demo updates when the user acts.
  std::string status_message = "ready";

  TranscriptViewOptions opts;
  opts.theme = theme;
  opts.follow = true;
  opts.on_copy = [&](std::string text) {
    const std::size_t len = text.size();
    status_message = "Copied " + std::to_string(len) + " chars";
  };
  opts.on_open_details = [&](std::size_t index) {
    status_message = "Opened block #" + std::to_string(index);
  };
  TranscriptView view(&model, std::move(opts));

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  // Seed the deterministic demo content.
  for (TranscriptItem& block : BuildSeedBlocks()) {
    model.append(std::move(block));
  }
  model.begin_tail(TextBlock{""});

  std::atomic<bool> streaming{true};
  std::atomic<std::size_t> token_cursor{0};

  // The generator thread only schedules repaints; the actual model mutation
  // happens on the main thread when the Custom event is handled, so the model
  // is never mutated concurrently with rendering.
  std::thread generator([&] {
    while (streaming.load(std::memory_order_relaxed)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(45));
      screen.PostEvent(ftxui::Event::Custom);
    }
  });

  auto view_component = view.component();

  ftxui::Component root = ftxui::Renderer(view_component, [&] {
    std::string stream_state = streaming.load(std::memory_order_relaxed) ? "STREAMING" : "paused";
    std::string follow_state = view.follow() ? "FOLLOW" : "manual";
    const std::size_t bookmarks = model.bookmarks().size();

    ftxui::Element viewport = view_component->Render() | ftxui::flex;
    if (view.search_mode()) {
      viewport = ftxui::window(ftxui::text(" search: type query, Enter/Esc to close "), viewport);
    }

    return ftxui::vbox({
               ftxui::text("Terminal UI Kit — TranscriptView (coding-agent transcript)") |
                   ftxui::bold,
               ftxui::hbox({
                   ftxui::text("[" + stream_state + "]") |
                       (streaming.load(std::memory_order_relaxed)
                            ? ftxui::color(ftxui::Color::Green)
                            : ftxui::color(ftxui::Color::GrayDark)),
                   ftxui::text("  follow: " + follow_state) |
                       (view.follow() ? ftxui::color(ftxui::Color::Green)
                                      : ftxui::color(ftxui::Color::GrayDark)),
                   ftxui::text("  blocks: " + std::to_string(model.block_count())),
                   ftxui::text("  bookmarks: " + std::to_string(bookmarks)),
                   ftxui::text("  search matches: " + std::to_string(model.match_count())),
                   ftxui::text("  "),
                   ftxui::text(status_message) | ftxui::dim,
               }),
               ftxui::separator(),
               viewport,
               ftxui::separator(),
               KeyHintBar({{"space", "start/stop stream"},
                           {"f", "follow"},
                           {"/", "search"},
                           {"n/N", "next/prev"},
                           {"b", "bookmark"},
                           {"y", "copy"},
                           {"enter", "details"},
                           {"g/G", "begin/end"},
                           {"l", "100k blocks"},
                           {"q/esc", "exit"}},
                          theme),
           }) |
           ftxui::border;
  });

  root |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Custom) {
      // One streaming step, executed on the main thread.
      if (!streaming.load(std::memory_order_relaxed)) {
        return true;
      }
      if (!model.has_tail()) {
        model.begin_tail(TextBlock{""});
      }
      const std::size_t cursor = token_cursor.fetch_add(1, std::memory_order_relaxed);
      model.append_tail(kTokenBank[cursor % (sizeof(kTokenBank) / sizeof(kTokenBank[0]))]);
      // Occasionally finalize the tail into a completed block and start a new
      // one, simulating multi-turn agent output accumulating heterogeneous
      // blocks over time.
      if (token_cursor.load(std::memory_order_relaxed) % 40 == 0) {
        model.finalize_tail();
        model.begin_tail(TextBlock{""});
      }
      return true;
    }
    if (event == ftxui::Event::Character(" ")) {
      streaming.store(!streaming.load(std::memory_order_relaxed));
      return true;
    }
    if (event == ftxui::Event::Character("l")) {
      if (model.has_tail()) {
        model.finalize_tail();
      }
      for (std::size_t i = 0; i < 100000; ++i) {
        model.append(RotatingBlock(i));
      }
      status_message = "Generated 100,000 blocks";
      return true;
    }
    if (event == ftxui::Event::Character("q")) {
      screen.ExitLoopClosure()();
      return true;
    }
    if (event == ftxui::Event::Escape && !view.search_mode()) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);

  streaming.store(false);
  generator.join();
  return 0;
}