#include "terminal_ui_kit/diff/unified_diff_parser.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "terminal_ui_kit/core/text_style.h"

namespace terminal_ui_kit {

// Утилита для обрезки пробельных символов.
static std::string_view Trim(std::string_view s) {
  s.remove_prefix(std::min(s.find_first_not_of(" \t"), s.size()));
  size_t last_not = s.find_last_not_of(" \t");
  if (last_not != std::string_view::npos) {
    s.remove_suffix(s.size() - last_not - 1);
  }
  return s;
}

// Разделение текста на строки.
static std::vector<std::string_view> SplitLines(std::string_view text) {
  std::vector<std::string_view> lines;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t newline = text.find('\n', pos);
    if (newline == std::string_view::npos) {
      lines.push_back(text.substr(pos));
      break;
    } else {
      lines.push_back(text.substr(pos, newline - pos));
      pos = newline + 1;
    }
  }
  return lines;
}

std::pair<bool, DiffFile> UnifiedDiffParser::Parse(std::string_view diff_text) const {
  DiffFile result;

  auto lines = SplitLines(diff_text);
  if (lines.empty()) {
    return {false, DiffFile{}};
  }

  size_t idx = 0;

  // Пропускаем index строку если есть
  if (idx < lines.size()) {
    std::string_view trimmed = Trim(lines[idx]);
    if (trimmed.starts_with("index ")) {
      idx++;
    }
  }

  // Парсим пути к файлам
  if (idx < lines.size()) {
    std::string_view trimmed = Trim(lines[idx]);
    if (trimmed.starts_with("---")) {
      result.old_path = std::string(trimmed.substr(3));
      result.old_path.erase(0, result.old_path.find_first_not_of(" \t"));
      idx++;
    }
  }

  if (idx < lines.size()) {
    std::string_view trimmed = Trim(lines[idx]);
    if (trimmed.starts_with("+++")) {
      result.new_path = std::string(trimmed.substr(3));
      result.new_path.erase(0, result.new_path.find_first_not_of(" \t"));
      idx++;
    }
  }

  // Парсим hunks
  while (idx < lines.size()) {
    std::string_view trimmed = Trim(lines[idx]);

    if (trimmed.starts_with("@@")) {
      // Начало ханка
      DiffHunk hunk;
      hunk.header = std::string(lines[idx]);

      idx++;

      // Парсим строки ханка
      while (idx < lines.size()) {
        std::string_view line = lines[idx];

        // Проверка на следующий hunk или конец diff
        if (line.substr(0, 2) == "@@") {
          break;
        }

        if (line.empty()) {
          idx++;
          continue;
        }

        DiffLine diff_line = ParseLine(line);
        hunk.lines.push_back(std::move(diff_line));
        idx++;
      }

      result.hunks.push_back(std::move(hunk));
    } else {
      idx++;
    }
  }

  return {true, result};
}

DiffLine UnifiedDiffParser::ParseLine(std::string_view line) const {
  DiffLine result;
  result.old_line = std::nullopt;
  result.new_line = std::nullopt;

  if (line.empty()) {
    result.type = DiffLineType::kContext;
    return result;
  }

  char prefix = line[0];
  std::string_view content;

  switch (prefix) {
    case ' ':
      // Контекстная строка: пробел - это префикс типа, текст идет дальше
      // В unified diff контекстные строки имеют формат " <text>" и пробел
      // сохраняется как часть содержимого (это визуальный отступ в diff)
      result.type = DiffLineType::kContext;
      content = line;
      break;
    case '+':
      // Добавленная строка: текст без ведущего пробела
      result.type = DiffLineType::kAddition;
      content = line.substr(1);
      break;
    case '-':
      // Удаленная строка: текст без ведущего пробела
      result.type = DiffLineType::kDeletion;
      content = line.substr(1);
      break;
    case '\\':
      // Continuation line for no newline at end of file
      result.type = DiffLineType::kContext;
      content = line;
      break;
    default:
      result.type = DiffLineType::kContext;
      content = line;
      break;
  }

  // Добавляем содержимое как StyledText
  result.content.append({std::string(content), TextStyle{}});

  return result;
}

}  // namespace terminal_ui_kit