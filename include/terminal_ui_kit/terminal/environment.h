#pragma once

#include <optional>
#include <string>

namespace terminal_ui_kit {
namespace terminal {

// Abstract access to the process environment. Detection logic reads through
// this interface so it can be exercised deterministically against an injected
// snapshot in tests instead of the real process environment.
class EnvironmentProvider {
 public:
  virtual ~EnvironmentProvider() = default;

  // Returns the value of the variable `name`, or nullopt when the variable is
  // not set. A variable that is set but empty yields an empty string (not
  // nullopt), so callers can distinguish "absent" from "present but empty"
  // where that distinction matters (e.g. NO_COLOR).
  virtual std::optional<std::string> Get(const std::string& name) const = 0;
};

// EnvironmentProvider backed by the real process environment via std::getenv.
// The returned value is copied into an owned std::string immediately, so the
// provider never exposes a pointer that a later environment mutation could
// invalidate.
class SystemEnvironment : public EnvironmentProvider {
 public:
  std::optional<std::string> Get(const std::string& name) const override;
};

}  // namespace terminal
}  // namespace terminal_ui_kit