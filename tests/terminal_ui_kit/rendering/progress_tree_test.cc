#include "terminal_ui_kit/components/progress_tree.h"

#include <ftxui/component/event.hpp>

#include "terminal_ui_kit/testing/virtual_screen.h"
#include <gtest/gtest.h>

namespace terminal_ui_kit {
namespace {

TEST(ProgressTree, RendersNestedTaskLabels) {
  ProgressTask child;
  child.id = "test";
  child.label = "Tests";
  child.status = Status::kRunning;

  ProgressTask root;
  root.id = "build";
  root.label = "Build";
  root.status = Status::kPending;
  root.children = {child};

  ftxui::Component tree = ProgressTree({root}, default_dark_theme());
  std::string text = test_support::render_to_text(tree->Render(), 40, 3);

  EXPECT_NE(text.find("Build"), std::string::npos);
  EXPECT_NE(text.find("Tests"), std::string::npos);
}

TEST(ProgressTree, RendersProgressAndSpinnerForRunningTasks) {
  ProgressTask known;
  known.id = "download";
  known.label = "Download";
  known.status = Status::kRunning;
  known.fraction = 0.5;

  ProgressTask unknown;
  unknown.id = "scan";
  unknown.label = "Scan";
  unknown.status = Status::kRunning;

  ftxui::Component tree = ProgressTree({known, unknown}, default_dark_theme());
  std::string text = test_support::render_to_text(tree->Render(), 60, 3);

  EXPECT_NE(text.find("50%"), std::string::npos);
  EXPECT_NE(text.find("|"), std::string::npos);
}

TEST(ProgressTree, SpaceTogglesRootChildren) {
  ProgressTask root;
  root.id = "build";
  root.label = "Build";
  ProgressTask child;
  child.id = "test";
  child.label = "Tests";
  root.children = {child};
  ftxui::Component tree = ProgressTree({root}, default_dark_theme());

  tree->OnEvent(ftxui::Event::Character(' '));
  std::string text = test_support::render_to_text(tree->Render(), 40, 3);

  EXPECT_EQ(text.find("Tests"), std::string::npos);
}

TEST(ProgressTree, DownThenSpaceTogglesSelectedChild) {
  ProgressTask grandchild;
  grandchild.id = "assert";
  grandchild.label = "Assertions";
  ProgressTask child;
  child.id = "test";
  child.label = "Tests";
  child.children = {grandchild};
  ProgressTask root;
  root.id = "build";
  root.label = "Build";
  root.children = {child};
  ftxui::Component tree = ProgressTree({root}, default_dark_theme());

  tree->OnEvent(ftxui::Event::ArrowDown);
  tree->OnEvent(ftxui::Event::Character(' '));
  std::string text = test_support::render_to_text(tree->Render(), 40, 4);

  EXPECT_NE(text.find("Tests"), std::string::npos);
  EXPECT_EQ(text.find("Assertions"), std::string::npos);
}

TEST(ProgressTree, EndSelectsLastVisibleRow) {
  ProgressTask first;
  first.id = "first";
  first.label = "First";
  ProgressTask last;
  last.id = "last";
  last.label = "Last";
  ProgressTask hidden;
  hidden.id = "child";
  hidden.label = "Hidden";
  ProgressTask nested;
  nested.id = "nested";
  nested.label = "Nested";
  hidden.children = {nested};
  last.children = {hidden};
  ftxui::Component tree = ProgressTree({first, last}, default_dark_theme());

  tree->OnEvent(ftxui::Event::End);
  tree->OnEvent(ftxui::Event::ArrowUp);
  tree->OnEvent(ftxui::Event::Character(' '));
  std::string text = test_support::render_to_text(tree->Render(), 40, 4);

  EXPECT_NE(text.find("Hidden"), std::string::npos);
  EXPECT_EQ(text.find("Nested"), std::string::npos);
}

TEST(ProgressTreeModel, PreservesExpandedStateAcrossSnapshots) {
  ProgressTask first_child;
  first_child.id = "test";
  first_child.label = "Tests";
  ProgressTask first_root;
  first_root.id = "build";
  first_root.label = "Build";
  first_root.children = {first_child};

  ProgressTreeModel model(default_dark_theme());
  model.set_tasks({first_root});
  model.component()->OnEvent(ftxui::Event::Character(' '));

  ProgressTask second_child = first_child;
  ProgressTask second_root = first_root;
  second_root.children = {second_child};
  model.set_tasks({second_root});
  std::string text = test_support::render_to_text(model.component()->Render(), 40, 3);

  EXPECT_EQ(text.find("Tests"), std::string::npos);
}

}  // namespace
}  // namespace terminal_ui_kit
