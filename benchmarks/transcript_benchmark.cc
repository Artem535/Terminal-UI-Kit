#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "terminal_ui_kit/components/transcript.h"
#include "terminal_ui_kit/components/transcript_model.h"
#include <benchmark/benchmark.h>

namespace terminal_ui_kit {
namespace {

constexpr std::size_t kBlockCount = 100000;
constexpr int kViewportWidth = 100;
constexpr int kViewportHeight = 30;

// Appending 100k completed blocks must stay linear and fast.
void BM_AppendOneHundredThousandBlocks(benchmark::State& state) {
  for (auto _ : state) {
    TranscriptModel model;
    for (std::size_t i = 0; i < kBlockCount; ++i) {
      model.append(TextBlock{"message " + std::to_string(i)});
    }
    benchmark::DoNotOptimize(model.block_count());
  }
}
BENCHMARK(BM_AppendOneHundredThousandBlocks);

// Streaming into the mutable tail must not create a new entry and must stay
// fast regardless of the preceding transcript size (no full relayout).
void BM_StreamingTailAppendNoNewEntries(benchmark::State& state) {
  constexpr std::size_t kChunks = 10000;
  for (auto _ : state) {
    TranscriptModel model;
    for (std::size_t i = 0; i < 100000; ++i) {
      model.append(TextBlock{"prefix " + std::to_string(i)});
    }
    model.begin_tail(TextBlock{""});
    const std::size_t count_before = model.block_count();
    for (std::size_t i = 0; i < kChunks; ++i) {
      model.append_tail("tok ");
    }
    benchmark::DoNotOptimize(model.block_count());
    if (model.block_count() != count_before) {
      state.SkipWithError("tail append created a new entry");
      break;
    }
  }
}
BENCHMARK(BM_StreamingTailAppendNoNewEntries);

// Rendering a viewport-sized slice of a 100k-block transcript must only touch
// visible cells.
void BM_RenderViewportOfLargeTranscript(benchmark::State& state) {
  TranscriptModel model;
  for (std::size_t i = 0; i < kBlockCount; ++i) {
    model.append(TextBlock{"message " + std::to_string(i)});
  }
  TranscriptViewOptions options;
  options.follow = false;
  TranscriptView view(&model, std::move(options));
  ftxui::Screen screen(kViewportWidth, kViewportHeight);
  // Establish the layout box once, outside the timed loop.
  ftxui::Render(screen, view.component()->Render());

  for (auto _ : state) {
    ftxui::Render(screen, view.component()->Render());
    benchmark::DoNotOptimize(screen);
  }
}
BENCHMARK(BM_RenderViewportOfLargeTranscript);

// Search builds its index once per (query, revision); repeated identical
// searches after no data change must not rebuild.
void BM_SearchIndexBuildOverLargeTranscript(benchmark::State& state) {
  TranscriptModel model;
  for (std::size_t i = 0; i < kBlockCount; ++i) {
    model.append(TextBlock{"message " + std::to_string(i)});
  }
  // Warm and rebuild the index.
  for (auto _ : state) {
    benchmark::DoNotOptimize(model.find("message"));
    // Force a rebuild by changing the query each iteration.
    benchmark::DoNotOptimize(model.find(std::to_string(state.iterations() % 100)));
  }
}
BENCHMARK(BM_SearchIndexBuildOverLargeTranscript);

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();