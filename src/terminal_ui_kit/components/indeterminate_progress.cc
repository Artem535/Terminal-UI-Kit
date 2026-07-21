#include "terminal_ui_kit/components/indeterminate_progress.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include <ftxui/component/component_base.hpp>
#include <ftxui/component/animation.hpp>
#include <ftxui/dom/elements.hpp>

#include "terminal_ui_kit/components/style_bridge.h"

namespace terminal_ui_kit {
namespace {

std::pair<std::string_view, std::string_view> glyphs(ProgressStyle style) {
  switch (style) {
    case ProgressStyle::kUnicodeBlocks:
      return {"█", "░"};
    case ProgressStyle::kAscii:
      return {"#", "-"};
    case ProgressStyle::kDots:
      return {"●", "○"};
    case ProgressStyle::kBraille:
      return {"⠿", "⠁"};
  }
  return {"█", "░"};
}

class IndeterminateProgressBase : public ftxui::ComponentBase {
 public:
  IndeterminateProgressBase(const Theme& theme, ProgressBarOptions options, std::size_t segment_width,
                            std::chrono::milliseconds frame_duration)
      : theme_(theme), options_(options), segment_width_(segment_width), frame_duration_(frame_duration) {}

 private:
  ftxui::Element Render() override {
    ftxui::animation::RequestAnimationFrame();
    const std::size_t filled = std::min(segment_width_, options_.width);
    const auto [filled_glyph, empty_glyph] = glyphs(options_.style);
    ftxui::Elements elements;
    for (std::size_t index = 0; index < options_.width; ++index) {
      const bool active = index >= offset_ && index < offset_ + filled;
      elements.push_back(ftxui::text(std::string(active ? filled_glyph : empty_glyph)) |
                         to_decorator(active ? theme_.accent : theme_.muted));
    }
    return ftxui::hbox(std::move(elements));
  }
  bool Focusable() const override { return false; }
  void OnAnimation(ftxui::animation::Params& params) override {
    elapsed_ += params.duration();
    while (elapsed_ >= frame_duration_) {
      elapsed_ -= frame_duration_;
      if (options_.width > 0) offset_ = (offset_ + 1) % options_.width;
    }
    ftxui::animation::RequestAnimationFrame();
  }
  Theme theme_;
  ProgressBarOptions options_;
  std::size_t segment_width_;
  std::chrono::milliseconds frame_duration_;
  std::size_t offset_ = 0;
  ftxui::animation::Duration elapsed_ = std::chrono::milliseconds(0);
};

}  // namespace

ftxui::Component IndeterminateProgress(const Theme& theme, ProgressBarOptions options,
                                       std::size_t segment_width,
                                       std::chrono::milliseconds frame_duration) {
  return ftxui::Make<IndeterminateProgressBase>(theme, options, segment_width, frame_duration);
}

}  // namespace terminal_ui_kit
