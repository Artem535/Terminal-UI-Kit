#include "terminal_ui_kit/components/spinner.h"

#include <cstddef>
#include <utility>

#include <ftxui/component/animation.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

namespace terminal_ui_kit {
namespace {

class SpinnerBase : public ftxui::ComponentBase {
 public:
  SpinnerBase(int charset_index, std::chrono::milliseconds frame_duration)
      : charset_index_(charset_index), frame_duration_(frame_duration) {}

 private:
  ftxui::Element Render() override {
    ftxui::animation::RequestAnimationFrame();
    return ftxui::spinner(charset_index_, frame_);
  }

  bool Focusable() const override { return false; }

  void OnAnimation(ftxui::animation::Params& params) override {
    elapsed_ += params.duration();
    while (elapsed_ >= frame_duration_) {
      elapsed_ -= frame_duration_;
      ++frame_;
    }
    ftxui::animation::RequestAnimationFrame();
  }

  int charset_index_;
  std::chrono::milliseconds frame_duration_;
  std::size_t frame_ = 0;
  ftxui::animation::Duration elapsed_ = std::chrono::milliseconds(0);
};

}  // namespace

ftxui::Component Spinner(int charset_index, std::chrono::milliseconds frame_duration) {
  return ftxui::Make<SpinnerBase>(charset_index, frame_duration);
}

}  // namespace terminal_ui_kit
