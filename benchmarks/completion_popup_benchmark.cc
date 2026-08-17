#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/components/completion_popup.h"
#include <benchmark/benchmark.h>

namespace terminal_ui_kit {
namespace {

// A large synthetic dataset (mimics a big symbol table or API surface) used to
// exercise fuzzy filtering and popup rendering at scale. Every label contains
// the benchmark query as a subsequence, so filtering must scan all of them.
std::vector<CompletionItem> MakeLargeDataset(std::size_t count) {
  std::vector<CompletionItem> data;
  data.reserve(count);
  for (std::size_t index = 0; index < count; ++index) {
    CompletionItem item;
    item.label = "symbol_" + std::to_string(index);
    item.category = "library";
    item.description = "generated symbol " + std::to_string(index);
    item.insert_text = item.label;
    data.push_back(std::move(item));
  }
  return data;
}

// Fuzzy filtering over a large provider result set, then rendering the popup.
void BenchmarkCompletionPopupFuzzyFilterAndRender(benchmark::State& state) {
  const std::size_t kItemCount = 10000;
  const int kWidth = 120;
  const int kHeight = 12;

  auto dataset = MakeLargeDataset(kItemCount);
  auto provider = std::make_shared<SyncCompletionProvider>(
      [&dataset](const CompletionContext&) { return dataset; });
  CompletionPopupOptions options;
  options.provider = provider;
  options.max_visible_rows = 8;

  CompletionPopup popup(ftxui::Renderer([] { return ftxui::text(">"); }), options);
  ftxui::Component component = popup.component();
  ftxui::Screen screen(kWidth, kHeight);

  popup.set_query("sym", 3);  // filters all 10k items, keeps them all
  ftxui::Render(screen, component->Render());

  for (auto _ : state) {
    ftxui::Render(screen, component->Render());
    benchmark::DoNotOptimize(screen);
  }
  state.counters["items"] = static_cast<double>(kItemCount);
}

BENCHMARK(BenchmarkCompletionPopupFuzzyFilterAndRender);

// The fuzzy subsequence matcher alone over a large corpus of labels.
void BenchmarkCompletionPopupFuzzyMatch(benchmark::State& state) {
  const std::size_t kItemCount = 10000;
  auto dataset = MakeLargeDataset(kItemCount);
  std::size_t matches = 0;
  for (auto _ : state) {
    for (const auto& item : dataset) {
      if (CompletionPopup::FuzzyMatch("sym", item.label)) {
        ++matches;
      }
    }
    benchmark::DoNotOptimize(matches);
  }
  state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()) *
                          static_cast<std::int64_t>(kItemCount));
}

BENCHMARK(BenchmarkCompletionPopupFuzzyMatch);

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();
