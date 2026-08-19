#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/components/unified_diff_view.h"
#include "terminal_ui_kit/diff/diff_model.h"
#include <benchmark/benchmark.h>

namespace terminal_ui_kit {
namespace {

// Builds a single-file diff model containing `line_count` context lines plus
// one added line, so the hunk has `line_count + 1` lines. The header path is
// deterministic and the data is retained by the view (no pointers/spans).
std::vector<diff::DiffFile> MakeLargeDiff(std::size_t line_count) {
  diff::DiffFile file;
  file.old_path = "big.txt";
  file.new_path = "big.txt";
  diff::DiffHunk hunk;
  hunk.header =
      "@@ -1," + std::to_string(line_count) + " +1," + std::to_string(line_count + 1) + " @@";
  for (std::size_t i = 1; i <= line_count; ++i) {
    diff::DiffLine line;
    line.type = diff::DiffLineType::kContext;
    line.old_line = static_cast<int>(i);
    line.new_line = static_cast<int>(i);
    line.content.append(TextSpan{"line " + std::to_string(i), TextStyle{}, std::nullopt});
    hunk.lines.push_back(std::move(line));
  }
  diff::DiffLine added;
  added.type = diff::DiffLineType::kAdded;
  added.new_line = static_cast<int>(line_count) + 1;
  added.content.append(TextSpan{"+ trailing added line", TextStyle{}, std::nullopt});
  hunk.lines.push_back(std::move(added));
  file.hunks.push_back(std::move(hunk));

  std::vector<diff::DiffFile> files;
  files.push_back(std::move(file));
  return files;
}

// Renders a 100,000-line diff and measures per-frame cost. A virtualized view
// must not rebuild the flat layout every frame and must only materialize the
// viewport rows, so repeated renders stay fast and cheap regardless of the
// 100,000-line model.
void BenchmarkUnifiedDiffViewRender(benchmark::State& state) {
  constexpr std::size_t kLineCount = 100000;
  constexpr int kViewportWidth = 120;
  constexpr int kViewportHeight = 40;

  UnifiedDiffView view({});
  view.SetFiles(MakeLargeDiff(kLineCount));
  ftxui::Screen screen(kViewportWidth, kViewportHeight);

  // Two warm-up passes: the virtual list only reports its real visible range
  // once its observed box has been populated (mirroring the rendering tests).
  ftxui::Render(screen, view.component()->Render());
  ftxui::Render(screen, view.component()->Render());
  const std::size_t initial_layout_builds = view.layout_build_count();
  const auto [first, count] = view.visible_range();
  if (count > static_cast<std::size_t>(kViewportHeight)) {
    state.SkipWithError("visible rows exceeded viewport bound");
    return;
  }

  for (auto _ : state) {
    ftxui::Render(screen, view.component()->Render());
    benchmark::DoNotOptimize(screen);
  }

  // The flat layout must not be rebuilt across renders with unchanged model
  // and layout constraints.
  if (view.layout_build_count() != initial_layout_builds) {
    state.SkipWithError("flat layout rebuilt across renders");
  }
  state.counters["total_rows"] = static_cast<double>(kLineCount);
  state.counters["visible_rows"] = static_cast<double>(count);
  state.counters["viewport_height"] = static_cast<double>(kViewportHeight);
  state.counters["layout_builds"] = static_cast<double>(view.layout_build_count());
}

BENCHMARK(BenchmarkUnifiedDiffViewRender)->Unit(benchmark::kMillisecond)->Iterations(50);

// Measures the one-time cost of loading a new 100,000-line model (parse + flat
// layout build). Reported once per iteration so a large diff stays interactive.
void BenchmarkUnifiedDiffViewLoad(benchmark::State& state) {
  constexpr std::size_t kLineCount = 100000;
  for (auto _ : state) {
    UnifiedDiffView view({});
    view.SetFiles(MakeLargeDiff(kLineCount));
    benchmark::DoNotOptimize(view);
  }
}

BENCHMARK(BenchmarkUnifiedDiffViewLoad)->Unit(benchmark::kMillisecond)->Iterations(5);

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();
