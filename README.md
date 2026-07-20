# Terminal UI Kit

[Русская версия](README_RU.md)

**Terminal UI Kit** is a C++20/23 library of high-level, production-ready
components for modern interactive terminal applications, built on top of
[FTXUI](https://github.com/ArthurSonzogni/FTXUI).

Full documentation (product requirements, contributing, security, changelog)
lives in [`docs/`](docs/modules/ROOT/pages/index.adoc) as AsciiDoc, built into
a site with [Antora](https://antora.org):

```sh
npm ci
npm run docs:build   # writes build/site/index.html
```

GitHub also renders the `.adoc` pages directly if you'd rather read them
in-repo, starting from [`docs/modules/ROOT/pages/index.adoc`](docs/modules/ROOT/pages/index.adoc).

## Building the library

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

## License

MIT — see [LICENSE](LICENSE).
