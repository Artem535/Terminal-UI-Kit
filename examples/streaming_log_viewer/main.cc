#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/key_hint_bar.h"
#include "terminal_ui_kit/components/log_view.h"
#include "terminal_ui_kit/document/ansi_parser.h"
#include "terminal_ui_kit/document/log_model.h"
#include "terminal_ui_kit/theme/theme.h"

int main() {
  using namespace terminal_ui_kit;
  using namespace std::chrono_literals;

  const Theme& theme = default_dark_theme();
  LogModel log_model;
  std::mutex log_mutex;
  std::atomic<bool> running{true};

  LogViewOptions opts;
  opts.log_model = &log_model;
  opts.theme = theme;
  LogView view(std::move(opts));

  auto screen = ftxui::ScreenInteractive::Fullscreen();

  std::thread generator([&] {
    const LogSeverity sev_map[] = {LogSeverity::kTrace, LogSeverity::kDebug, LogSeverity::kInfo,
                                   LogSeverity::kWarning, LogSeverity::kError};
    const char* messages[] = {
        "\x1b[32mConnection\x1b[0m established to 10.0.0.1:8080",
        "Received \x1b[33m42\x1b[0m bytes from upstream",
        "\x1b[31mError\x1b[0m: timeout waiting for acknowledgement",
        "Processing batch \x1b[36m#1024\x1b[0m",
        "Cache hit ratio: \x1b[32m87.3%\x1b[0m",
        "\x1b[33mWarning\x1b[0m: memory usage at 78%",
        "Request \x1b[36mGET /api/v2/users\x1b[0m completed in 143ms",
        "Retry attempt \x1b[33m3/5\x1b[0m for job-id=abc-123",
        "\x1b[32mSuccess\x1b[0m: migration task finished",
        "Heartbeat check: \x1b[32mOK\x1b[0m",
    };
    constexpr std::size_t kMsgCount = sizeof(messages) / sizeof(messages[0]);

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    while (running) {
      std::this_thread::sleep_for(200ms + std::chrono::milliseconds(std::rand() % 300));

      auto now = std::chrono::system_clock::now();
      auto tt = std::chrono::system_clock::to_time_t(now);
      struct tm tm_storage;
      localtime_r(&tt, &tm_storage);
      char timestamp[16];
      std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &tm_storage);

      int sev_idx = std::rand() % 5;
      LogEntry entry;
      entry.timestamp = timestamp;
      entry.severity = sev_map[sev_idx];
      entry.message = parse_ansi(messages[std::rand() % kMsgCount]);

      {
        std::lock_guard<std::mutex> lock(log_mutex);
        log_model.append(std::move(entry));
      }
      screen.PostEvent(ftxui::Event::Custom);
    }
  });

  ftxui::Component root = ftxui::Renderer(view.component(), [&] {
    return ftxui::vbox({
               ftxui::text("Terminal UI Kit - Streaming Log Viewer") | ftxui::bold,
               ftxui::text("Watch live logs from a simulated backend") | ftxui::dim,
               ftxui::separator(),
               view.component()->Render() | ftxui::flex,
               ftxui::separator(),
               ftxui::text(std::string("Follow: ") + (view.follow() ? "ON" : "OFF")) |
                   (view.follow() ? ftxui::color(ftxui::Color::Green)
                                  : ftxui::color(ftxui::Color::GrayDark)),
               KeyHintBar({{"up/down", "scroll"}, {"end", "follow"}, {"q/ESC", "quit"}}, theme),
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