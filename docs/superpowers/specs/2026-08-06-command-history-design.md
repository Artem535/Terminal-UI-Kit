# CommandHistory — design spec

## Problem

Input and editor components need a small, reusable, bounded command history: a
model that stores recent commands, supports Up/Down-style navigation, and can
filter/persist it. The core model must stay independent of FTXUI so it is
usable (and testable) with no terminal backend.

## Design

Header-only module `terminal_ui_kit/command/` exposing:

- `CommandHistory` — the model.
  - Storage: `std::deque<std::string>` (owned), oldest -> newest, bounded by a
    configurable capacity. Capacity `0` disables retention (`Add` is a no-op).
  - `Add` ignores empty / whitespace-only commands and consecutive duplicates
    (exact match with the most recent entry). Submitting any command (including
    one ignored as blank or a duplicate) resets the navigation cursor to "at
    the end" (behind the newest entry).
  - `Previous()`/`Next()`: deterministic boundary behavior. `Previous()` at the
    oldest entry returns it again (cursor never goes below 0); on empty history
    returns `nullopt`. `Next()` past the newest entry (or already there)
    returns `nullopt`. Results are returned as owning `std::optional<std::string>`.
  - `Search`/`SearchPrefix`: substring/prefix match over retained entries,
    returned oldest -> newest (chronological, stable order).
  - `Clear`: empties storage, resets navigation, forwards to persistence.
  - Eviction: oldest entry popped when the capacity is already reached.
- `CommandHistoryPersistence` — abstract adapter with `Save(const std::string&)`
  (pure) and `Clear()` (defaults to success). The model never lets a persistence
  error mutate in-memory history.
- `CommandSensitivePolicy` — abstract predicate `IsSensitive(string_view)`;
  concrete `NeverSensitivePolicy` and `PrefixSensitivePolicy`. Sensitive
  commands are kept in memory but never written to persistence.
  `SetSensitivePolicy`/`SetPersistence` allow runtime replacement (used by the
  example to toggle sensitive mode).

## Decisions

- Owned `std::string` storage; never retain `std::string_view`/pointers.
- Header-only keeps the model trivially embeddable and avoids a new `.cc`/ABI.
- Search/eviction are linear; documented as fine for typical command-history
  sizes (small, interactive).
- No internal synchronization (documented; callers synchronize if needed).

## Example

An FTXUI interactive app (`examples/command_history_example.cpp`) demonstrating
add, Up/Down navigation, substring & prefix search, size/capacity display,
clear, sensitive-mode toggle, and eviction, using only the public API.