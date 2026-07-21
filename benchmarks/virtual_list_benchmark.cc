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

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();
