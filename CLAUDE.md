# cna-template — Claude Code Guidelines

## Project Overview

**cna-template** is a starter/template repository for building applications
on **CNA**, a C++ reimplementation of the XNA 4.0 game framework programming
model, built on SDL3 with a pluggable graphics backend layer. It ships a
minimal example (`HelloGame`) meant to be deleted and replaced by whatever
the template's user is actually building.

CNA lives in a sibling repository:
```
../cna
```
For CNA-specific coding rules (namespaces, XNA API compliance, C# property
conventions, type aliases, events, visibility mapping, etc.) see
`../cna/CLAUDE.md`. Those rules apply to CNA's own internals; this file only
covers what's relevant to code living in *this* repository.

**sharp-runtime** is the C++ reimplementation of .NET base-class library
types (`System::Math`, `System::String`, `EventHandler`, `IDisposable`,
primitive type aliases, etc.) that CNA depends on:
```
../sharp-runtime
```

**cna-samples** is a large collection of official XNA 4.0 C# samples already
ported to CNA/C++ — the best reference when porting XNA code and unsure of
the CNA equivalent of a specific API:
```
../cna-samples
```

## Build

```bash
cmake --preset easygl && cmake --build --preset easygl
```

See `README.md` for the full list of presets/backends/platforms. Sibling
checkouts `../cna`, `../sharp-runtime`, and (for `EASYGL`) `../easy-gl` must
exist before configuring — see `CNA_ROOT_DIR` in `CMakeLists.txt` if yours
live somewhere other than the default `../cna`.

## Source layout

```
include/HelloGame/    - HelloGame.hpp (delete/replace with your own game)
src/HelloGame/         - HelloGame.cpp, Program.cpp (entry point)
Content/                - game assets (PNG/WAV/OGG/etc. -- never .xnb)
android/                - Gradle project, points at this repo's own CMakeLists.txt
cmake/toolchains/       - MinGW-w64 cross-compilation toolchain file
```

## Code rules

- Do not change XNA API names or signatures when calling into CNA — match
  CNA/FNA exactly.
- Every concrete `Game` subclass needs `GetTypeNameHPP()` in its header and
  `GetTypeNameCPP(ClassName, "ClassName")` at file scope in its `.cpp` — see
  `HelloGame` for the pattern. This is CNA/sharp-runtime bookkeeping, not
  XNA, but required to compile.
- Properties are getter/setter pairs (`getFooProperty()`/`setFooProperty()`),
  never raw public fields, matching CNA's own convention.
- This template must keep working on **all 5 CNA graphics backends**
  (`SDL_RENDERER`, `EASYGL`, `BGFX`, `VULKAN`, `WEBGPU`) and on **all 4
  platforms** (Linux, Windows, Web, Android). Concretely: avoid any XNA API
  usage that only works on 3D-capable backends unless you have a reason
  HelloGame specifically needs it — e.g. use `GraphicsDevice::Clear(r,g,b,a)`
  rather than `Clear(const Color&)`, since the latter requires depth/stencil
  clear support that the 2D-only `SDL_RENDERER` backend (mandatory on
  Android/Web) does not have and throws on. This was found by actually
  building and running HelloGame against SDL_RENDERER, not by reading the
  header — when touching graphics code here, prefer testing against
  `SDL_RENDERER` specifically, since it's the most restrictive backend and
  the one every platform must support.
- Do not add features or abstractions beyond what's requested. This is a
  template — keep `HelloGame` genuinely minimal so it stays easy to delete
  and replace.
- Do not add comments that explain *what* the code does — only *why*, when
  non-obvious (e.g. the `Clear()` overload choice above).

## Assets

CNA never reads `.xnb`. See the "Porting a C# XNA 4.0 game" section of
`README.md` for the full asset-conversion table and tooling references in
`../cna-samples/tools/`.
