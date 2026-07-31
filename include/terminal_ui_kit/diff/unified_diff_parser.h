#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/diff/diff_model.h"

namespace terminal_ui_kit {

// Парсер unified diff формата (PRD section 28.2).
// Принимает строку unified diff и возвращает структуру DiffFile.
class UnifiedDiffParser {
 public:
  // Разбор diff-строки. Возвращает пару (успех, результат).
  // Если разбор не удался, результат содержит информацию о первой ошибке.
  [[nodiscard]] std::pair<bool, DiffFile> Parse(std::string_view diff_text) const;

 private:
  // Парсинг одной строки diff и определение её типа.
  [[nodiscard]] DiffLine ParseLine(std::string_view line) const;

  // Извлечение путей из заголовка файла.
  [[nodiscard]] std::pair<std::string, std::string> ParseFileHeaders(
      std::vector<std::string_view> lines) const;

  // Извлечение номеров строк из заголовка ханка.
  [[nodiscard]] std::optional<std::pair<int, int>> ParseHunkRange(std::string_view range_str) const;
};

}  // namespace terminal_ui_kit