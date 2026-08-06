#include "terminal_ui_kit/terminal/environment.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

namespace terminal_ui_kit {
namespace terminal {

std::optional<std::string> ProcessEnvironment::Get(std::string_view name) const {
  // std::getenv takes a null-terminated C string; build one from the view.
  const std::string key(name);
  const char* value = std::getenv(key.c_str());
  if (value == nullptr) {
    return std::nullopt;
  }
  return std::string(value);
}

MapEnvironment::MapEnvironment(std::unordered_map<std::string, std::string> values)
    : values_(std::move(values)) {}

void MapEnvironment::Set(std::string name, std::string value) {
  values_.insert_or_assign(std::move(name), std::move(value));
}

void MapEnvironment::Unset(std::string_view name) { values_.erase(std::string(name)); }

std::optional<std::string> MapEnvironment::Get(std::string_view name) const {
  const auto it = values_.find(std::string(name));
  if (it == values_.end()) {
    return std::nullopt;
  }
  return it->second;
}

}  // namespace terminal
}  // namespace terminal_ui_kit