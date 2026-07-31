#pragma once

#include <optional>
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

  // Утилита для обрезки пробельных символов.
  [[nodiscard]] static std::string_view Trim(std::string_view s);
};

}  // namespace terminal_ui_kit