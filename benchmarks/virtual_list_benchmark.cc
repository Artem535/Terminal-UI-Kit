#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/components/virtual_list.h"
#include <benchmark/benchmark.h>

namespace terminal_ui_kit {
namespace {

void BenchmarkVirtualListRenderAndPaint(benchmark::State& state) {
  constexpr std::size_t kItemCount = 100000;
  constexpr int kViewportWidth = 120;
  constexpr int kViewportHeight = 40;

  std::size_t rendered_items = 0;
  VirtualListOptions options;
  options.item_count = [] { return kItemCount; };
  options.item_height = 1;
  options.render_item = [&rendered_items](std::size_t index, int) {
    ++rendered_items;
    return ftxui::text(std::to_string(index));
  };
  VirtualListModel model(std::move(options));
  ftxui::Component component = model.component();
  ftxui::Screen screen(kViewportWidth, kViewportHeight);

  ftxui::Render(screen, component->Render());
  rendered_items = 0;

  for (auto _ : state) {
    ftxui::Render(screen, component->Render());
    benchmark::DoNotOptimize(screen);
  }

  state.counters["rendered_items"] =
      static_cast<double>(rendered_items) / static_cast<double>(state.iterations());
}

BENCHMARK(BenchmarkVirtualListRenderAndPaint);

void BenchmarkVariableHeightVirtualListRenderAndPaint(benchmark::State& state) {
  constexpr std::size_t kItemCount = 100000;
  constexpr int kViewportWidth = 120;
  constexpr int kViewportHeight = 40;
  constexpr std::size_t kMaxRenderedItems = kViewportHeight + 1;

  std::size_t rendered_items = 0;
  std::size_t max_rendered_items = 0;
  VirtualListOptions options;
  options.item_count = [] { return kItemCount; };
  options.estimate_height = [](std::size_t index, int) { return static_cast<int>(index % 3) + 1; };
  options.render_item = [&rendered_items](std::size_t index, int) {
    ++rendered_items;
    const std::string row = "Row " + std::to_string(index);
    switch (index % 3) {
      case 0:
        return ftxui::text(row);
      case 1:
        return ftxui::vbox({ftxui::text(row), ftxui::text("  detail")});
      default:
        return ftxui::vbox({ftxui::text(row), ftxui::text("  detail"), ftxui::text("  more")});
    }
  };
  VirtualListModel model(std::move(options));
  ftxui::Component component = model.component();
  ftxui::Screen screen(kViewportWidth, kViewportHeight);

  ftxui::Render(screen, component->Render());
  rendered_items = 0;

  for (auto _ : state) {
    rendered_items = 0;
    ftxui::Render(screen, component->Render());
    max_rendered_items = std::max(max_rendered_items, rendered_items);
    benchmark::DoNotOptimize(screen);
  }

  state.counters["rendered_items"] = static_cast<double>(max_rendered_items);
  state.counters["viewport_height"] = static_cast<double>(kViewportHeight);
  if (max_rendered_items > kMaxRenderedItems) {
    state.SkipWithError("variable-height render exceeded viewport bound");
  }
}

BENCHMARK(BenchmarkVariableHeightVirtualListRenderAndPaint);

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();
