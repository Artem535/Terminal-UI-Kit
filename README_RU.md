# Terminal UI Kit

[English version](README.md)

**Terminal UI Kit** — библиотека высокоуровневых, готовых к production
компонентов для современных интерактивных терминальных приложений на
C++20/23, построенная поверх [FTXUI](https://github.com/ArthurSonzogni/FTXUI).

Библиотека предоставляет компоненты, которых намеренно нет в FTXUI:
виртуализированные списки и документы, потоковый вывод, многострочный
редактор, рендеринг Markdown и подсветку синтаксиса, просмотр диффов,
просмотр логов, автодополнение, командную палитру и терминальные интеграции
(буфер обмена, изображения, определение возможностей терминала).

Компоненты возвращают обычные `ftxui::Component` и `ftxui::Element`, библиотека
не запускает собственный event loop и не зависит от конкретного прикладного
домена — полное описание требований см. в [`doc/prd.md`](doc/prd.md).

## Статус

Ранняя стадия — создан только каркас репозитория (pre-0.1.0). Компоненты ещё
не реализованы; дорожная карта — раздел 57 PRD, план первых pull request'ов —
раздел 63.

## Сборка

CMake — основная система сборки, Xmake — дополнительный фронтенд для
разработки на Linux.

### CMake

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Опции фич описаны в [`cmake/TerminalUiKitOptions.cmake`](cmake/TerminalUiKitOptions.cmake).
Все опциональные зависимости и подпроекты tests/examples/benchmarks по
умолчанию выключены (`OFF`), поэтому обычный конфигурейшн не требует сети.

### Xmake

```sh
xmake config --mode=debug --yes
xmake build
```

## Структура репозитория

```text
include/terminal_ui_kit/   Публичные заголовки, по одной директории на модуль
src/terminal_ui_kit/       Реализация
tests/terminal_ui_kit/     Unit, rendering, interaction, integration, fuzz тесты
examples/                  Примеры приложений (чат, diff viewer, log viewer, ...)
benchmarks/                Замеры производительности
docs/                      Документация по компонентам и архитектуре
cmake/                     Вспомогательные CMake-модули
doc/                       Плановые документы (PRD)
```

## Документация

- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
- [Changelog](CHANGELOG.md)

## Лицензия

MIT — см. [LICENSE](LICENSE).
