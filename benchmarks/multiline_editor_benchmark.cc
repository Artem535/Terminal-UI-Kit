#include <cstddef>
#include <string>
#include <vector>

#include "terminal_ui_kit/editor/editor_document.h"
#include <benchmark/benchmark.h>

namespace terminal_ui_kit {
namespace {

// Guards against accidental quadratic behavior when inserting a large
// multi-line block (bracketed paste) into a large document.
void BenchmarkMultilineInsertLargePaste(benchmark::State& state) {
  const std::size_t kLines = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    EditorDocument doc;
    std::vector<std::string> lines(20000, "base line");
    doc = EditorDocument(std::move(lines));
    std::string paste;
    for (std::size_t i = 0; i < kLines; ++i) {
      paste += "p" + std::to_string(i) + "\n";
    }
    doc.set_cursor({10000, 0});
    doc.insert_text(paste);
    benchmark::DoNotOptimize(doc.text());
    benchmark::ClobberMemory();
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BenchmarkMultilineInsertLargePaste)->RangeMultiplier(4)->Range(256, 1 << 12);

// Guards against accidental quadratic behavior when typing many single
// characters into one line.
void BenchmarkTypingSingleLine(benchmark::State& state) {
  const std::size_t kChars = static_cast<std::size_t>(state.range(0));
  for (auto _ : state) {
    EditorDocument doc;
    for (std::size_t i = 0; i < kChars; ++i) {
      doc.insert_text("x");
    }
    benchmark::DoNotOptimize(doc.text());
    benchmark::ClobberMemory();
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BenchmarkTypingSingleLine)->RangeMultiplier(4)->Range(256, 1 << 14);

// Serialize the whole buffer; used to track text() cost at scale.
void BenchmarkBufferSerialize(benchmark::State& state) {
  const std::size_t kLines = static_cast<std::size_t>(state.range(0));
  EditorDocument doc;
  std::vector<std::string> lines;
  lines.reserve(kLines);
  for (std::size_t i = 0; i < kLines; ++i) {
    lines.push_back("serialize line content");
  }
  doc = EditorDocument(std::move(lines));
  for (auto _ : state) {
    std::string text = doc.text();
    benchmark::DoNotOptimize(text);
  }
  state.SetComplexityN(state.range(0));
}
BENCHMARK(BenchmarkBufferSerialize)->RangeMultiplier(4)->Range(1 << 10, 1 << 14);

}  // namespace
}  // namespace terminal_ui_kit

BENCHMARK_MAIN();
