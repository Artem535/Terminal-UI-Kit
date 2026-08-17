#include "terminal_ui_kit/components/completion_popup.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

using State = CompletionPopup::State;

CompletionItem MakeItem(std::string label, std::string category = {},
                        std::string description = {}) {
  CompletionItem item;
  item.label = std::move(label);
  item.category = std::move(category);
  item.description = std::move(description);
  return item;
}

ftxui::Component InputHost() {
  return ftxui::Renderer([] { return ftxui::text("input>"); });
}

// Deterministic sync data set used by most rendering tests. Fuzzy filtering is
// disabled so the supplied items render verbatim (filtering itself is covered
// by unit tests).
CompletionPopup MakeResultsPopup(const std::vector<CompletionItem>& data) {
  auto provider =
      std::make_shared<SyncCompletionProvider>([data](const CompletionContext&) { return data; });
  CompletionPopupOptions options;
  options.provider = provider;
  options.fuzzy_filter = false;
  CompletionPopup popup(InputHost(), options);
  popup.set_query("ab", 2);
  return popup;
}

// Renders one frame to establish layout/box, then a second frame to read the
// adjusted result (the box observer reports on the frame after it is laid out).
std::string RenderTwoFrames(ftxui::Component component, int width, int height) {
  test_support::render_to_screen(component->Render(), width, height);
  return test_support::render_to_text(component->Render(), width, height);
}

TEST(CompletionPopupRender, PopupBelowShowsHostThenItems) {
  CompletionPopup popup = MakeResultsPopup({MakeItem("alpha"), MakeItem("able")});
  popup.set_available_space(5, 0);  // plenty below => kBelow
  EXPECT_EQ(popup.placement(), CompletionPopup::Placement::kBelow);
  const std::string text = RenderTwoFrames(popup.component(), 40, 4);
  EXPECT_NE(text.find("input>"), std::string::npos);
  EXPECT_NE(text.find("alpha"), std::string::npos);
  EXPECT_NE(text.find("able"), std::string::npos);
  // The input line renders before the item lines in below-mode.
  EXPECT_LT(text.find("input>"), text.find("alpha"));
}

TEST(CompletionPopupRender, PopupAboveShowsItemsThenHost) {
  CompletionPopup popup = MakeResultsPopup({MakeItem("alpha"), MakeItem("able")});
  popup.set_available_space(0, 5);  // no room below, room above => kAbove
  EXPECT_EQ(popup.placement(), CompletionPopup::Placement::kAbove);
  const std::string text = RenderTwoFrames(popup.component(), 40, 4);
  EXPECT_NE(text.find("alpha"), std::string::npos);
  EXPECT_NE(text.find("able"), std::string::npos);
  // The item lines render before the input line in above-mode.
  EXPECT_LT(text.find("alpha"), text.find("input>"));
}

TEST(CompletionPopupRender, NarrowViewportFallsBackToFewerRows) {
  CompletionPopup popup =
      MakeResultsPopup({MakeItem("alpha"), MakeItem("able"), MakeItem("actor"), MakeItem("arrow")});
  popup.set_available_space(0, 0);  // narrow terminal: clamp to what fits
  // A two-row viewport can hold the host row plus a single popup row; the rest
  // of the results are truncated without crashing.
  const std::string text = RenderTwoFrames(popup.component(), 40, 2);
  EXPECT_NE(text.find("input>"), std::string::npos);
  EXPECT_NE(text.find("alpha"), std::string::npos);  // first result stays visible
  EXPECT_EQ(text.find("arrow"), std::string::npos);  // later rows truncated
}

TEST(CompletionPopupRender, MaxVisibleRowsCapsRowCount) {
  auto provider = std::make_shared<SyncCompletionProvider>([](const CompletionContext&) {
    std::vector<CompletionItem> items;
    for (int i = 0; i < 5; ++i) {
      items.push_back(MakeItem("item" + std::to_string(i)));
    }
    return items;
  });
  CompletionPopupOptions options;
  options.provider = provider;
  options.max_visible_rows = 2;
  options.fuzzy_filter = false;
  CompletionPopup popup(InputHost(), options);
  popup.set_query("a", 1);
  const std::string text = RenderTwoFrames(popup.component(), 40, 6);
  EXPECT_NE(text.find("item0"), std::string::npos);
  EXPECT_NE(text.find("item1"), std::string::npos);
  EXPECT_EQ(text.find("item2"), std::string::npos);  // capped by max_visible_rows
  EXPECT_NE(text.find("…"), std::string::npos);      // continuation marker
}

TEST(CompletionPopupRender, ResizeAdaptsRenderedRows) {
  CompletionPopup popup =
      MakeResultsPopup({MakeItem("alpha"), MakeItem("able"), MakeItem("actor"), MakeItem("arrow")});
  test_support::render_to_screen(popup.component()->Render(), 40, 5);
  const std::string tall = test_support::render_to_text(popup.component()->Render(), 40, 5);
  EXPECT_NE(tall.find("alpha"), std::string::npos);
  EXPECT_NE(tall.find("able"), std::string::npos);
  // Shrink the viewport: a two-row window keeps only the first popup row.
  test_support::render_to_screen(popup.component()->Render(), 40, 2);
  const std::string narrow = test_support::render_to_text(popup.component()->Render(), 40, 2);
  EXPECT_NE(narrow.find("alpha"), std::string::npos);
  EXPECT_EQ(narrow.find("able"), std::string::npos);  // dropped after the shrink
  EXPECT_EQ(narrow.find("arrow"), std::string::npos);
}

TEST(CompletionPopupRender, CategoriesAndDescriptionsRender) {
  CompletionPopup popup = MakeResultsPopup({MakeItem("connect", "core", "open a connection")});
  const std::string text = RenderTwoFrames(popup.component(), 60, 3);
  EXPECT_NE(text.find("connect"), std::string::npos);
  EXPECT_NE(text.find("core"), std::string::npos);
  EXPECT_NE(text.find("open a connection"), std::string::npos);
}

TEST(CompletionPopupRender, SelectedRowIsInverted) {
  CompletionPopup popup = MakeResultsPopup({MakeItem("alpha"), MakeItem("able")});
  popup.set_available_space(5, 0);
  test_support::render_to_screen(popup.component()->Render(), 40, 4);
  const ftxui::Screen screen = test_support::render_to_screen(popup.component()->Render(), 40, 4);
  EXPECT_EQ(popup.selected_index(), 0U);
  EXPECT_TRUE(screen.PixelAt(0, 1).inverted);  // first item is selected
  EXPECT_FALSE(screen.PixelAt(0, 2).inverted);
}

TEST(CompletionPopupRender, LoadingStateRowRenders) {
  struct ManualAsyncProvider : ICompletionProvider {
    void complete(const CompletionContext&, std::uint64_t,
                  const std::function<void(std::uint64_t, CompletionResult)>&) override {}
  } provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(InputHost(), options);
  popup.set_query("ab", 2);
  EXPECT_EQ(popup.state(), State::kLoading);
  const std::string text = RenderTwoFrames(popup.component(), 40, 3);
  EXPECT_NE(text.find("Loading"), std::string::npos);
}

TEST(CompletionPopupRender, NoResultsStateRowRenders) {
  struct ManualAsyncProvider : ICompletionProvider {
    std::uint64_t generation = 0;
    std::function<void(std::uint64_t, CompletionResult)> deliver;
    void complete(const CompletionContext&, std::uint64_t gen,
                  const std::function<void(std::uint64_t, CompletionResult)>& d) override {
      this->generation = gen;
      deliver = d;
    }
  } provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(InputHost(), options);
  popup.set_query("zz", 2);
  CompletionResult result;
  provider.deliver(provider.generation, std::move(result));
  EXPECT_EQ(popup.state(), State::kNoResults);
  const std::string text = RenderTwoFrames(popup.component(), 40, 3);
  EXPECT_NE(text.find("No matches"), std::string::npos);
}

TEST(CompletionPopupRender, ErrorStateRowRenders) {
  struct ManualAsyncProvider : ICompletionProvider {
    std::uint64_t generation = 0;
    std::function<void(std::uint64_t, CompletionResult)> deliver;
    void complete(const CompletionContext&, std::uint64_t gen,
                  const std::function<void(std::uint64_t, CompletionResult)>& d) override {
      this->generation = gen;
      deliver = d;
    }
  } provider;
  CompletionPopupOptions options;
  options.provider = std::shared_ptr<ICompletionProvider>(&provider, [](ICompletionProvider*) {});
  CompletionPopup popup(InputHost(), options);
  popup.set_query("ab", 2);
  CompletionResult result;
  result.error = "backend down";
  provider.deliver(provider.generation, std::move(result));
  EXPECT_EQ(popup.state(), State::kError);
  const std::string text = RenderTwoFrames(popup.component(), 40, 3);
  EXPECT_NE(text.find("Error"), std::string::npos);
  EXPECT_NE(text.find("backend down"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Interaction: keyboard-driven navigation and acceptance
// ---------------------------------------------------------------------------

TEST(CompletionPopupInteraction, ArrowDownMovesSelection) {
  CompletionPopup popup =
      MakeResultsPopup({MakeItem("alpha"), MakeItem("able"), MakeItem("actor")});
  EXPECT_EQ(popup.selected_index(), 0U);
  EXPECT_TRUE(popup.component()->OnEvent(ftxui::Event::ArrowDown));
  EXPECT_EQ(popup.selected_index(), 1U);
  popup.component()->OnEvent(ftxui::Event::ArrowDown);
  EXPECT_EQ(popup.selected_index(), 2U);
}

TEST(CompletionPopupInteraction, EnterAcceptsSelectedItem) {
  std::string accepted;
  auto provider = std::make_shared<SyncCompletionProvider>(
      [](const CompletionContext&) { return std::vector<CompletionItem>{MakeItem("save")}; });
  CompletionPopupOptions options;
  options.provider = provider;
  options.on_accept = [&accepted](const CompletionItem& item) { accepted = item.label; };
  CompletionPopup popup(InputHost(), options);
  popup.set_query("sa", 2);
  EXPECT_TRUE(popup.component()->OnEvent(ftxui::Event::Return));
  EXPECT_EQ(accepted, "save");
  EXPECT_FALSE(popup.visible());
}

TEST(CompletionPopupInteraction, TabAcceptsSelectedItemAndEscapeCancels) {
  auto provider = std::make_shared<SyncCompletionProvider>(
      [](const CompletionContext&) { return std::vector<CompletionItem>{MakeItem("save")}; });
  CompletionPopupOptions options;
  options.provider = provider;
  std::string accepted;
  options.on_accept = [&accepted](const CompletionItem& item) { accepted = item.label; };
  CompletionPopup popup(InputHost(), options);
  popup.set_query("sa", 2);
  EXPECT_TRUE(popup.component()->OnEvent(ftxui::Event::Tab));
  EXPECT_EQ(accepted, "save");
  EXPECT_FALSE(popup.visible());
  // Open again, then cancel with Escape.
  popup.set_query("sa", 2);
  EXPECT_TRUE(popup.visible());
  EXPECT_TRUE(popup.component()->OnEvent(ftxui::Event::Escape));
  EXPECT_FALSE(popup.visible());
}

TEST(CompletionPopupInteraction, NavigationKeysLeavePopupWhenHidden) {
  CompletionPopup popup = MakeResultsPopup({MakeItem("alpha")});
  popup.hide();
  // When hidden, navigation is not consumed: the host is left to handle it.
  EXPECT_FALSE(popup.component()->OnEvent(ftxui::Event::ArrowDown));
}

}  // namespace
}  // namespace terminal_ui_kit
