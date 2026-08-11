# Upstream issues affecting cna-template

Bugs and gaps in repositories this template consumes. Those repositories are
read-only from here: nothing below has been patched upstream.

**Re-audited: 2026-08-11**, against:

| Repo | Branch | SHA |
| --- | --- | --- |
| cna | develop | `7a64362efef4119bf880459ef1704fb2c52199e2` |
| sharp-runtime | develop | `f827a6c5349234d5ac938886788ed8eca8fe1c10` |
| easy-gl | develop | `0b46d35c394a9fb6aea6a85c6587894b5013da33` |
| meta-gl | develop | `571d3a62fe166b9781ac6193d137b12ff3757620` |
| free-direct | develop | `934f72ff0c52902631fceeb52c006f3ed2767485` |

Every entry carried over from the previous audit was re-verified rather than
assumed. Entries that turned out to be fixed, obsolete or simply wrong are kept
below, marked as such, because knowing a thing was *checked* is worth as much as
the finding itself.

---

## Open

### CNA-1 — `CMAKE_SOURCE_DIR` is used where CNA's own root is meant (blocker)

**Severity: blocks every downstream consumer.**

CNA is designed to be consumed with `add_subdirectory()`, which makes
`CMAKE_SOURCE_DIR` the **consumer's** top-level directory, not CNA's. Three
places in CNA's production build use it as if it meant CNA's own root:

| Location | Effect on a consumer |
| --- | --- |
| `modules/CMakeLists.txt:188` | Aborts the configure if the *consumer* has a top-level `src/` or `include/` directory, with the message "legacy global 'src/' tree reappeared at the repository root". |
| `modules/content/CMakeLists.txt:21` | `cna_content` compiles with the wrong cgltf include path → `fatal error: cgltf.h: No such file or directory`. |
| `modules/content/CMakeLists.txt:22` | Same for `stb`. |

The first was introduced by cna commit `33088cba6` ("build(modules): localize
module CMake ownership"). There is no option to disable any of them.

This is not theoretical, and it is not specific to this template. The sibling
`cna-examples` repository has a top-level `src/` *and* does
`add_subdirectory(../cna CNA_BUILD)`, so it is broken the same way;
`mobile-eggbert` has both `src/` and `include/`.

**Fix upstream:** resolve these against CNA's own root rather than the build's
top level — `${CMAKE_CURRENT_SOURCE_DIR}/..` from `modules/`, or a variable set
once at CNA's top level (`CNA_SOURCE_DIR`), or `PROJECT_SOURCE_DIR` guarded for
the nested case.

**What this template does meanwhile** (both visible, neither hides a failure):

- Keeps application sources in `game/src` and `game/include`, with a preflight
  check in `CMakeLists.txt` that explains the real cause if a top-level `src/`
  or `include/` reappears.
- Adds CNA's real `third_party/cgltf` and `third_party/stb` paths with
  `include_directories()` before `add_subdirectory()`, since subdirectories
  inherit that directory property. This repairs the search path without touching
  CNA.

Also present, but only reachable with `CNA_BUILD_TESTS=ON` (which the template
forces OFF), so not currently blocking: `cmake/Harnesses.cmake:205-211` and
`cmake/UnitTests.cmake:323+` build helper-script paths from
`${CMAKE_SOURCE_DIR}/scripts/...`, plus many `modules/*/examples/CMakeLists.txt`.

### CNA-2 — `CNA_GL_PROFILE_*` does not reach consumers, so the renderer misreports itself

CNA propagates `CNA_RENDERER_<X>` to consumers through the `cna_build_flags`
INTERFACE target (`modules/CMakeLists.txt:62-70`), but applies
`CNA_GL_PROFILE_<X>` with a directory-scoped `add_compile_definitions()`
(`cmake/RendererSelection.cmake:560`) that never reaches a parent-scope consumer.

All five GL renderers share the define `CNA_RENDERER_EASYGL`, and the profile is
what tells them apart. Without it, `CNA::getCurrentGraphicsRendererName()`
compiled in a consumer's translation unit falls through to its `OPENGLES3`
default (`modules/core/include/CNA/GraphicsRendererType.hpp:176`). So on a
`WEBGL2`, `WEBGL1`, `OPENGL33` or `OPENGLES2` build, the application reports
`"OPENGLES3"` while CNA's own translation units report the truth — a wrong answer
and an ODR hazard across the same binary.

**Fix upstream:** add `CNA_GL_PROFILE_${CNA_GRAPHICS_RENDERER}` to
`cna_build_flags` alongside `${CNA_RENDERER_DEFINE}`.

**What this template does meanwhile:** re-applies the profile define to its own
target when the selected renderer is in the GL family (`CMakeLists.txt`).

### CNA-3 — CNA's README §10 usage example is wrong and does not compile

`cna/README.md` §10 (the example at `:588-600`) still tells readers to call
`device.Present()` inside `Draw()`. It should not: `Game::EndDraw()` already
presents exactly once per frame, matching real XNA/FNA. Presenting twice makes
SDL show an invalid backbuffer, and the window flickers every frame. This
template's own `HelloGame` originally inherited the bug from that example.

Re-checking the snippet this time turned up two further defects in the same ~12
lines:

- it includes `"Microsoft/Xna/Framework/Graphics/GraphicsDeviceManager.hpp"`,
  but the real header has no `Graphics/` component;
- it uses an unqualified `CornflowerBlue`; only `Color::CornflowerBlue` exists
  (`modules/graphics/include/Microsoft/Xna/Framework/Color.hpp:108`).

As written, §10 does not compile.

### CNA-4 — CI job selects a renderer name that no longer exists

`cna/.github/workflows/input-ci.yml:106` still passes
`-DCNA_GRAPHICS_RENDERER=EASYGL`. `EASYGL` was retired as a selector when the GL
family split into five profile names; it now hits
`FATAL_ERROR "Unknown graphics renderer"` at
`cmake/RendererSelection.cmake:869`. That job cannot configure as written.

### CNA-5 — stale renderer counts inside CNA's own tooling and docs

- `cna/scripts/check_renderer_identities.py` says "exactly 42" in its docstring
  (`:4`, `:19`) while its own table holds 46. The table is right.
- Several `plan_*.md` files still describe `CNA_GRAPHICS_BACKEND` and reference
  `cmake/BackendSelection.cmake` / `cmake/BackendLibraries.cmake`, neither of
  which exists any more.
- `cmake/RendererSelection.cmake:122-124` points at `BackendLibraries.cmake` for
  the OPENGLES1 dependency gate; that gate now lives in the module.

Cosmetic, but they are exactly the sort of stale prose that misled this template
before.

### CNA-6 — `CNA_SOKOL_API` accepts a value its own error message contradicts

`cmake/RendererSelection.cmake:509-523`: the cache `STRINGS` list offers
`DIRECTX11`, and `:516` accepts it, but the option's docstring and the
unknown-value `FATAL_ERROR` both name `D3D11`. Anyone following the error message
picks a value that is then rejected.

### SHARP-RUNTIME-1 — unconditional `find_package(ZLIB REQUIRED)` on a MinGW cross-build

Previously filed against `sharp-runtime/CMakeLists.txt:5`; that line no longer
exists. After modularization the same call is at
`sharp-runtime/modules/io-compression/CMakeLists.txt:5`, and it is still reached
on every build, because components default to `All`
(`sharp-runtime/CMakeLists.txt:49-53`) and CNA never narrows
`SHARP_RUNTIME_COMPONENTS`.

**Status: reproduced live in this audit.** Cross-compile probe:

```bash
cmake -S . -B build-probe --toolchain cmake/toolchains/mingw-w64.cmake \
    -DCNA_GRAPHICS_RENDERER=SDL_RENDERER
```

fails with `Could NOT find ZLIB (missing: ZLIB_LIBRARY) (found version "1.3.1")`
from `sharp-runtime/modules/io-compression/CMakeLists.txt:5`, because
`find_package(ZLIB)` resolves the *host's* `zlib.h` (version string) but not a
MinGW-targeted `.a`, and `CMAKE_FIND_ROOT_PATH` in
`cna/cmake/toolchains/mingw-w64.cmake` does not point at one. A MinGW zlib
(`libz.a`) exists elsewhere on this machine outside any path the toolchain
searches, which is what let the version probe half-succeed and produced the
confusing "found version 1.3.1" in an otherwise-failing message.

Not fixed here: installing a system package or exporting `CMAKE_PREFIX_PATH`
would make it configure, but that changes host state rather than the template,
and the underlying issue — `sharp-runtime` pulling in `io-compression`
unconditionally instead of CNA declaring only the components it needs via
`SHARP_RUNTIME_COMPONENTS` — is upstream's to fix.

### MOBILE-EGGBERT-1 — stray `include_directories(... CNA)`

`mobile-eggbert/CMakeLists.txt:88` still has it. Separately, mobile-eggbert is
now stale in its own right (last commit 2026-07-16, still using
`CNA_GRAPHICS_BACKEND`), so it is **no longer a safe structural model** for this
template. It was used as one historically; it should not be again without
checking every claim against CNA first.

---

## Fixed upstream — verified still fixed

### `Clear(const Color&)` crashed on SDL_RENDERER

Fixed, and improved since: `modules/graphics/src/Xna/GraphicsDevice.cpp:415-425`
now masks depth and stencil independently via `SupportsDepthBuffer()` /
`SupportsStencilBuffer()` (`:492-504`). `Clear(const Color&)` is safe on every
renderer. The template no longer carries a workaround.

### Double renderer reconfiguration during startup

Fixed in `modules/runtime/src/GraphicsDeviceManager.cpp:60-79`. The fix survived
the modularization, and its comment still cites this file by name.

---

## Obsolete

### `CNA_GRAPHICS_BACKEND`'s `STRINGS` list omitted WEBGPU

Obsolete: the variable itself is gone (renamed `CNA_GRAPHICS_RENDERER`), and
`WEBGPU` is one of the 46 canonical values (`cmake/RendererSelection.cmake:16`).

---

## Mistaken

### `cna_copy_sdl_runtime()` is unusable by a downstream consumer

**Wrong, and the previous audit had already half-corrected itself in place.**
CMake functions are global once defined, so everything CNA `include()`s is
callable from the parent scope after `add_subdirectory()`.
`cna_copy_sdl_runtime()` (`cmake/ThirdPartySDL.cmake:323`),
`cna_copy_mingw_runtime()` (`:241`) and `cna_copy_mingw_cxx_runtime()` (`:290`)
all work; `cna-samples` relies on exactly that. The template now calls
`cna_copy_sdl_runtime()` on Windows, with a `TARGET_RUNTIME_DLLS` fallback.

The old entry's *stated reason* ("functions defined via `include()` while CNA is
processed as our subdirectory are not visible back in this top-level scope") was
simply false, and it had been copied into `CMakeLists.txt` as a comment. Both are
now corrected.

---

## Not re-verified in this audit

Listed so the next session does not mistake silence for a passing result:

- The MinGW cross-build (`SHARP-RUNTIME-1`) was not executed.
- The Android build was not executed: no Android SDK/NDK on this machine. CNA's
  own docs disagree with each other about its current state —
  `cna/docs/android-graphics-limitations.md:24-50` reports the NDK cross-compile
  failing inside sharp-runtime, while `cna/docs/devices-build.md:270-386` records
  an APK that built and ran on an emulator on 2026-07-05.
- The Emscripten build was not executed: no emsdk on this machine.
