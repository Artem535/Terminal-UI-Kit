# Terminal UI Kit

[Русская версия](README_RU.md)

**Terminal UI Kit** is a C++20/23 library of high-level, production-ready
components for modern interactive terminal applications, built on top of
[FTXUI](https://github.com/ArthurSonzogni/FTXUI).

It provides the components FTXUI intentionally leaves out: virtualized
lists and documents, streaming output, a multiline editor, Markdown and
syntax-highlighted code rendering, diff views, log views, completion and
command-palette UX, and terminal integrations (clipboard, images, capability
detection).

The library returns ordinary `ftxui::Component` and `ftxui::Element` objects,
does not run its own event loop, and stays independent of any specific
application domain — see [`doc/prd.md`](doc/prd.md) for the full product
requirements document.

## Status

Early scaffolding stage (pre-0.1.0). No components are implemented yet; see
the [PRD](doc/prd.md) section 57 for the milestone roadmap and section 63 for
the initial pull request plan.

## Building

CMake is the primary, authoritative build system; Xmake is available as a
secondary developer-facing frontend on Linux.

### CMake

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Feature options are documented in [`cmake/TerminalUiKitOptions.cmake`](cmake/TerminalUiKitOptions.cmake).
All optional dependencies and the test/example/benchmark subprojects default
to `OFF`, so a bare configure never touches the network.

### Xmake

```sh
xmake config --mode=debug --yes
xmake build
```

## Repository Layout

```text
include/terminal_ui_kit/   Public headers, one directory per module
src/terminal_ui_kit/       Implementation
tests/terminal_ui_kit/     Unit, rendering, interaction, integration, fuzz tests
examples/                  Example applications (chat, diff viewer, log viewer, ...)
benchmarks/                Performance benchmarks
docs/                      Component and architecture documentation
cmake/                     CMake helper modules
doc/                       Planning documents (PRD)
```

## Documentation

Component and architecture guides will land under `docs/` as each module is
implemented (see [`doc/prd.md`](doc/prd.md) section 53 for the planned set).
Available now:

- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
- [Changelog](CHANGELOG.md)

## License

MIT — see [LICENSE](LICENSE).
