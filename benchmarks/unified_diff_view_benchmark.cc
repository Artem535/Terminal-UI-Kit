#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/components/unified_diff_view.h"
#include "terminal_ui_kit/diff/diff_model.h"
#include "terminal_ui_kit/diff/unified_diff_parser.h"
#include <benchmark/benchmark.h>

namespace terminal_ui_kit {
namespace {

// Builds a unified diff string with roughly `total_lines` diff lines spread
// across a handful of files, so the model reaches the required 100k-line scale.
std::string make_large_diff(int total_lines) {
  constexpr int kFiles = 10;
  std::string diff;
  int per_file = total_lines / kFiles;
  if (per_file % 2 != 0) --per_file;  // keep old/new counts consistent
  for (int f = 0; f < kFiles; ++f) {
    const std::string name = "bench_" + std::to_string(f) + ".txt";
    diff += "diff --git a/" + name + " b/" + name + "\n";
    diff += "--- a/" + name + "\n";
    diff += "+++ b/" + name + "\n";
    // Each file emits `per_file/2` context lines and `per_file/2` added lines.
    const int context = per_file / 2;
    diff += "@@ -1," + std::to_string(context) + " +1," + std::to_string(per_file) + " @@\n";
    for (int i = 0; i < context; ++i) {
      diff += " line_" + std::to_string(i) + "\n";
      diff += "+added_" + std::to_string(i) + "\n";
    }
  }
  return diff;
}

void BenchmarkUnifiedDiffViewRenderAndPaint(benchmark::State& state) {
  constexpr int kViewportWidth = 120;
  constexpr int kViewportHeight = 40;
  const std::string diff_text = make_large_diff(100000);
  std::vector<diff::DiffFile> files = diff::UnifiedDiffParser{}.Parse(diff_text);

  UnifiedDiffView view(std::move(files));
  ftxui::Component component = view.component();
  ftxui::Screen screen(kViewportWidth, kViewportHeight);

  ftxui::Render(screen, component->Render());
  ftxui::Render(screen, component->Render());

  for (auto _ : state) {
    ftxui::Render(screen, component->Render());
    benchmark::DoNotOptimize(screen);
  }

  state.counters["row_count"] = static_cast<double>(view.row_count());
  state.counters["viewport_height"] = static_cast<double>(kViewportHeight);
}

BENCHMARK(BenchmarkUnifiedDiffViewRenderAndPaint);

void BenchmarkUnifiedDiffViewNavigation(benchmark::State& state) {
  constexpr int kViewportWidth = 120;
  constexpr int kViewportHeight = 40;
  const std::string diff_text = make_large_diff(100000);
  std::vector<diff::DiffFile> files = diff::UnifiedDiffParser{}.Parse(diff_text);

  UnifiedDiffView view(std::move(files));
  ftxui::Component component = view.component();
  ftxui::Screen screen(kViewportWidth, kViewportHeight);
  ftxui::Render(screen, component->Render());
  ftxui::Render(screen, component->Render());

  std::size_t i = 0;
  for (auto _ : state) {
    // Jump to progressively later rows, forcing the scroll window to move,
    // then render. Navigation + render stays interactive even at 100k lines.
    view.scroll_to_row(i * 137 % view.row_count());
    ftxui::Render(screen, component->Render());
    benchmark::DoNotOptimize(screen);
    ++i;
  }
  state.counters["row_count"] = static_cast<double>(view.row_count());
}

BENCHMARK(BenchmarkUnifiedDiffViewNavigation);

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();
