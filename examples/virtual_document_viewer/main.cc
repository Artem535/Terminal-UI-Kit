#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <thread>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/virtual_document.h"
#include "terminal_ui_kit/document/streaming_document.h"
#include "terminal_ui_kit/theme/theme.h"

namespace {

void copy_to_clipboard(const std::string& text) {
  const char* cmds[] = {"wl-copy", "xclip -selection clipboard", "pbcopy"};
  for (const char* cmd : cmds) {
    FILE* pipe = popen(cmd, "w");
    if (!pipe) continue;
    fwrite(text.data(), 1, text.size(), pipe);
    if (pclose(pipe) == 0) return;
  }
}

}  // namespace

int main() {
  using namespace terminal_ui_kit;
  using namespace std::chrono_literals;

  const Theme& theme = default_dark_theme();
  StreamingDocument doc;
  std::atomic<bool> running{true};

  VirtualDocumentOptions opts;
  opts.document = &doc;
  opts.theme = theme;
  opts.show_line_numbers = true;
  opts.on_copy = [](std::string text) { copy_to_clipboard(text); };
  VirtualDocument view(std::move(opts));

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  std::thread generator([&] {
    const char* words[] = {
        "Lorem",          "ipsum",       "dolor",       "sit",        "amet",
        "consectetur",    "adipiscing",  "elit",        "sed",        "do",
        "eiusmod",        "tempor",      "incididunt",  "ut",         "labore",
        "et",             "dolore",      "magna",       "aliqua",     "Ut",
        "enim",           "ad",          "minim",       "veniam",     "quis",
        "nostrud",        "exercitation", "ullamco",     "laboris",    "nisi",
        "ut",             "aliquip",     "ex",          "ea",         "commodo",
        "consequat",      "Duis",        "aute",        "irure",      "dolor",
        "in",             "reprehenderit", "in",         "voluptate",  "velit",
        "esse",           "cillum",      "dolore",      "eu",         "fugiat",
        "nulla",          "pariatur",    "Excepteur",   "sint",       "occaecat",
        "cupidatat",      "non",         "proident",    "sunt",       "in",
        "culpa",          "qui",         "officia",     "deserunt",   "mollit",
        "anim",           "id",          "est",         "laborum",
    };
    constexpr std::size_t kWordCount = sizeof(words) / sizeof(words[0]);

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    int line_num = 0;
    while (running) {
      std::this_thread::sleep_for(100ms + std::chrono::milliseconds(std::rand() % 200));

      std::string line = "Line " + std::to_string(++line_num) + ": ";
      const int word_count = 5 + std::rand() % 20;
      for (int w = 0; w < word_count; ++w) {
        if (w > 0) line += " ";
        line += words[std::rand() % kWordCount];
      }
      line += "\n";
      doc.append(line);
      screen.PostEvent(ftxui::Event::Custom);
    }
  });

  ftxui::Component root = ftxui::Renderer(view.component(), [&] {
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Virtual Document Viewer") | ftxui::bold,
               ftxui::text("Streaming wrapped text with follow mode") | ftxui::dim,
               ftxui::separator(),
               view.component()->Render() | ftxui::flex,
               ftxui::separator(),
               ftxui::text(std::string("Follow: ") + (view.follow() ? "ON" : "OFF")) |
                   (view.follow() ? ftxui::color(ftxui::Color::Green)
                                  : ftxui::color(ftxui::Color::GrayDark)),
               KeyHintBar({{"up/down", "scroll"},
                       {"end", "follow"},
                       {"v", "select"},
                       {"y", "yank"},
                       {"q/ESC", "quit"}},
                       theme),
           }) |
           ftxui::border;
  });

  root |= ftxui::CatchEvent([&](const ftxui::Event& event) {
    if (event == ftxui::Event::Character('q') || event == ftxui::Event::Escape) {
      screen.ExitLoopClosure()();
      return true;
    }
    return false;
  });

  screen.Loop(root);
  running = false;
  generator.join();
}