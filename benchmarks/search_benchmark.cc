// Benchmark: SearchableTextView
//
// Measures two things:
//   1. Search over a large document (literal and regex modes) via the
//      SearchEngine -- the cost of building the match index on demand.
//   2. Repeated re-rendering of a SearchableTextView with a fixed query, which
//      must NOT rebuild the search index. The match set is cached at set_query
//      time; each Render only reads it. This documents the performance
//      guarantee that plain re-renders do not re-scan the document.

#include <cstddef>
#include <string>
#include <vector>

#include "terminal_ui_kit/components/searchable_text_view.h"
#include "terminal_ui_kit/search/search_engine.h"
#include <benchmark/benchmark.h>

namespace {

std::vector<std::string> MakeLargeDocument(std::size_t line_count) {
  std::vector<std::string> lines;
  lines.reserve(line_count);
  for (std::size_t i = 0; i < line_count; ++i) {
    switch (i % 5) {
      case 0:
        lines.push_back("Lorem ipsum dolor sit amet target consectetur adipiscing elit");
        break;
      case 1:
        lines.push_back("sed do eiusmod tempor incididunt ut labore et dolore magna");
        break;
      case 2:
        lines.push_back("aliqua. Ut enim ad minim veniam target quis nostrud");
        break;
      case 3:
        lines.push_back("exercitation ullamco target laboris nisi ut aliquip ex ea");
        break;
      default:
        lines.push_back("commodo consequat. Duis aute irure dolor in reprehenderit");
        break;
    }
  }
  return lines;
}

void BM_SearchLargeDocumentLiteral(benchmark::State& state) {
  std::vector<std::string> document = MakeLargeDocument(static_cast<std::size_t>(state.range(0)));
  std::vector<std::string_view> views;
  views.reserve(document.size());
  for (const std::string& s : document) {
    views.push_back(s);
  }
  std::vector<terminal_ui_kit::TextMatch> matches;
  terminal_ui_kit::SearchOptions options;
  for (auto _ : state) {
    terminal_ui_kit::SearchEngine::search(views, "target", options, matches);
    benchmark::DoNotOptimize(matches.size());
  }
}
BENCHMARK(BM_SearchLargeDocumentLiteral)->RangeMultiplier(4)->Range(1000, 100000);

void BM_SearchLargeDocumentRegex(benchmark::State& state) {
  std::vector<std::string> document = MakeLargeDocument(static_cast<std::size_t>(state.range(0)));
  std::vector<std::string_view> views;
  views.reserve(document.size());
  for (const std::string& s : document) {
    views.push_back(s);
  }
  std::vector<terminal_ui_kit::TextMatch> matches;
  terminal_ui_kit::SearchOptions options;
  options.use_regex = true;
  for (auto _ : state) {
    terminal_ui_kit::SearchEngine::search(views, "[Tt]arget", options, matches);
    benchmark::DoNotOptimize(matches.size());
  }
}
BENCHMARK(BM_SearchLargeDocumentRegex)->RangeMultiplier(4)->Range(1000, 100000);

void BM_RepeatedRenderNoReindex(benchmark::State& state) {
  // Re-rendering with an unchanged query/document must not re-run the search;
  // the match set is computed once in set_query and cached.
  std::vector<std::string> document = MakeLargeDocument(static_cast<std::size_t>(state.range(0)));
  terminal_ui_kit::SearchableTextView view;
  view.set_lines(document);
  view.set_query("target");
  view.component()->Render();  // prime so the viewport is laid out
  for (auto _ : state) {
    benchmark::DoNotOptimize(view.component()->Render());
  }
  state.SetItemsProcessed(state.iterations());
  // The match count must stay stable across the render loop (cached, not
  // rebuilt): a regression that re-indexes would still return the same count,
  // but the timing reflects the difference.
  const std::size_t matches = view.match_count();
  if (matches == 0) {
    state.SkipWithError("expected cached matches");
  }
}
BENCHMARK(BM_RepeatedRenderNoReindex)->RangeMultiplier(4)->Range(1000, 100000);

}  // namespace

BENCHMARK_MAIN();