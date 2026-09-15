#include "terminal_ui_kit/terminal/environment.h"

#include <cstdlib>
#include <string>

namespace terminal_ui_kit {
namespace terminal {

std::optional<std::string> SystemEnvironment::Get(const std::string& name) const {
  const char* value = std::getenv(name.c_str());
  if (value == nullptr) return std::nullopt;
  return std::string(value);
}

}  // namespace terminal
}  // namespace terminal_ui_kit