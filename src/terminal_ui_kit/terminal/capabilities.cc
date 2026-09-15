#include "terminal_ui_kit/terminal/capabilities.h"

namespace terminal_ui_kit {
namespace terminal {

const char* ColorDepthToName(ColorDepth depth) noexcept {
  switch (depth) {
    case ColorDepth::kNone:
      return "none";
    case ColorDepth::kAnsi16:
      return "16";
    case ColorDepth::kAnsi256:
      return "256";
    case ColorDepth::kTrueColor:
      return "truecolor";
  }
  return "?";
}

}  // namespace terminal
}  // namespace terminal_ui_kit