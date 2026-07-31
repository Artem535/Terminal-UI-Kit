#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/core/styled_text.h"

namespace terminal_ui_kit {

// Типы строк в unified diff (PRD section 28.3).
enum class DiffLineType {
  kContext,     // 未 измененная строка (prefix ' ')
  kAddition,    // Добавленная строка (prefix '+')
  kDeletion,    // Удаленная строка (prefix '-')
  kFileHeader,  // Строки индекса/заголовка файла
  kSourcePath,  // Строки с путем к исходному файлу (prefix '---')
  kTargetPath,  // Строки с путем к целевому файлу (prefix '+++')
  kHunkHeader,  // Заголовок ханка (prefix '@@')
};

// Структура одной строки diff (PRD section 28.3).
struct DiffLine {
  DiffLineType type;
  std::optional<int> old_line;  // Номер строки в старом файле
  std::optional<int> new_line;  // Номер строки в новом файле
  StyledText content;           // Содержимое строки со стилями
};

// Структура одного ханка diff (блок изменений) (PRD section 28.3).
struct DiffHunk {
  std::string header;  // Текст заголовка @@ -old_start,count +new_start,count @@
  std::vector<DiffLine> lines;
};

// Структура одного файла в diff (PRD section 28.3).
struct DiffFile {
  std::string old_path;  // Путь к старому файлу
  std::string new_path;  // Путь к новому файлу
  std::vector<DiffHunk> hunks;
};

}  // namespace terminal_ui_kit