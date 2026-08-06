#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace terminal_ui_kit {
namespace terminal {

// Read-only access to environment variables during capability detection.
//
// Detection (TerminalDetector::Detect) is a pure function of the caller-
// supplied provider, so tests substitute a MapEnvironment (or any fake) and
// never depend on the real process environment. ProcessEnvironment is the one
// adapter that queries std::getenv; only the example's "real environment"
// preset uses it.
class EnvironmentProvider {
 public:
  virtual ~EnvironmentProvider() = default;

  // Returns the value of the named variable, or nullopt when it is unset.
  // An empty (but present) value is distinct from "unset".
  [[nodiscard]] virtual std::optional<std::string> Get(std::string_view name) const = 0;
};

// Reads the real process environment via std::getenv.
class ProcessEnvironment final : public EnvironmentProvider {
 public:
  [[nodiscard]] std::optional<std::string> Get(std::string_view name) const override;
};

// A deterministic, map-backed provider for tests and synthetic presets.
// Lookups are case-sensitive and exact; an explicitly-set empty value is a
// "present" variable.
class MapEnvironment final : public EnvironmentProvider {
 public:
  explicit MapEnvironment(std::unordered_map<std::string, std::string> values = {});

  void Set(std::string name, std::string value);
  void Unset(std::string_view name);

  [[nodiscard]] std::optional<std::string> Get(std::string_view name) const override;

 private:
  std::unordered_map<std::string, std::string> values_;
};

}  // namespace terminal
}  // namespace terminal_ui_kit