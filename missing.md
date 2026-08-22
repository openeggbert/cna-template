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

### CNA-7 — ancillary targets still link the legacy `SHARP_RUNTIME` umbrella

The modular sharp-runtime creates the compatibility target `SHARP_RUNTIME`
only when component `All` is selected
(`sharp-runtime/cmake/SharpRuntimeComponents.cmake:337-365`). CNA's framework
modules correctly use `cna_link_sharp_runtime()` and component targets, but
`cmake/ToolGltfToCnj.cmake:11-15` still links the old name directly. The tool is
built unconditionally. `Harnesses.cmake` and `UnitTests.cmake` contain more of
the same pattern behind disabled template options.

Once this template selected CNA's narrow component closure, a full Emscripten
build therefore reached the final tool link and failed with:

```text
wasm-ld: error: unable to find library -lSHARP_RUNTIME
```

**Fix upstream:** migrate these targets to `cna_link_sharp_runtime()` or the
specific `SharpRuntime::<Component>` targets they consume.

**What this template does meanwhile:** after CNA creates the selected component
targets, it supplies a compatibility `SHARP_RUNTIME` INTERFACE target over
CNA's own default component list. This preserves every legacy CNA tool without
re-enabling `All`.

### CNA-8 — CANVAS did not follow the `CreateRenderTargetCube` interface change

`IGraphicsRenderer::CreateRenderTargetCube()` gained a third
`preserveContents` argument. Older CNA revisions left CANVAS with the stale
four-argument declaration and definition, so Clang rejected its non-overriding
method marked `override` before application sources were linked.

**Resolved upstream:** current CNA includes `preserveContents` in both CANVAS
signatures. The 50-renderer verification confirmed that this form configures
and builds under Emscripten.

**Compatibility retained by this template:** for an older Emscripten CANVAS
checkout, the template still verifies both exact stale signatures, derives
corrected copies under the build tree, and compiles the renderer against an
overlay header without editing CNA. When the corrected signature is already
present, it skips the overlay. Unknown or inconsistent signature pairs remain
a fatal error so the workaround cannot silently outlive another API change.

### CNA-9 — vendored SDL's persistent cache defaults inside the source checkout

`cmake/ThirdPartySDL.cmake:20-31` chooses
`${CMAKE_CURRENT_SOURCE_DIR}/.sdl-prebuilt-<target>` and then configures, builds
and installs SDL there during the parent CMake configure. This fails when CNA is
consumed from a read-only checkout even though both the consumer source and
binary directories are writable.

`CNA_SDL_PREBUILT_ROOT` is a working escape hatch. The local web verification
used a persistent writable directory under `cna-template/build/` and both web
renderers shared it. The template now makes that its default for every target,
while preserving an explicit caller value. Upstream should default to a user
cache or binary-tree location, or at least detect a non-writable CNA source and
choose one, while retaining the explicit cache override.

### SHARP-RUNTIME-1 — default `All` selection pulls zlib into every CNA consumer

Previously filed against `sharp-runtime/CMakeLists.txt:5`; that line no longer
exists. After modularization the same call is at
`sharp-runtime/modules/io-compression/CMakeLists.txt:5`. It is correctly scoped
to `IO.Compression`, but sharp-runtime defaults to component `All`
(`sharp-runtime/CMakeLists.txt:49-53`) and CNA sets its actual component needs
only *after* `add_subdirectory(sharp-runtime)`. Every embedded CNA consumer
therefore reaches `find_package(ZLIB REQUIRED)` unless it anticipates CNA's
needs before adding CNA.

**Status: reproduced, then resolved at the consumer integration seam.** The raw
failing probe was:

```bash
cmake -S . -B build-probe --toolchain cmake/toolchains/mingw-w64.cmake \
    -DCNA_GRAPHICS_RENDERER=SDL_RENDERER
```

fails with `Could NOT find ZLIB (missing: ZLIB_LIBRARY) (found version "1.3.1")`
from `sharp-runtime/modules/io-compression/CMakeLists.txt:5`, because
`find_package(ZLIB)` resolves the *host's* `zlib.h` (version string) but not a
MinGW-targeted `.a`, and `CMAKE_FIND_ROOT_PATH` in
`cna-template/cmake/toolchains/mingw-w64.cmake` does not invent a dependency
prefix. Debian's installed `libz-mingw-w64` package contains runtime DLLs only,
not a development import/static archive. That lets the host header half-satisfy
the version probe and produces the confusing "found version 1.3.1" in an
otherwise-failing message.

Supplying a previously built Windows-target zlib proved that no later compiler
blocker was hidden behind the configure failure, but it was not the final
solution. The template now includes CNA's own
`cmake/SharpRuntimeConsumption.cmake` before `add_subdirectory(CNA)` and places
its `CNA_SHARP_RUNTIME_DEFAULT_COMPONENTS` into
`SHARP_RUNTIME_COMPONENTS`—only when the application has not made an explicit
selection. The final clean-cache-equivalent command needed no prefix:

```bash
cmake -S . -B build-probe --toolchain cmake/toolchains/mingw-w64.cmake \
    -DCNA_GRAPHICS_RENDERER=SDL_RENDERER
cmake --build build-probe --parallel 3
```

That configured without any ZLIB cache entry, compiled all 435 steps and linked
`HelloGame.exe`. The same selection reduced and unblocked both Emscripten
builds. This is still an upstream ordering issue: CNA already owns the right
list, but must apply it before adding sharp-runtime. Once it does, the template
preload can be removed.

### SHARP-RUNTIME-2 — Emscripten rejects an unused native-only helper

`modules/io/src/System/IO/RandomAccess.cpp:91-93` defines `NativeDetail()` and
`ThrowNative()` outside the platform branches. Every use of `ThrowNative()` is
inside `_WIN32` or POSIX branches; Emscripten's branches throw
`PlatformNotSupportedException` directly. Clang therefore reports
`ThrowNative` as unused, and the component's own `-Werror` promotes it to a
build failure.

**Fix upstream:** guard the native helpers with `#if !defined(__EMSCRIPTEN__)`
or mark the intentionally unavailable helper `[[maybe_unused]]`.

**What this template does meanwhile:** append
`-Wno-error=unused-function` to this one source in target `sharp_runtime_io` on
Emscripten. The warning remains visible; only its promotion to an error is
disabled. No warning policy is weakened for the application, CNA, or any other
Sharp Runtime source.

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
are callable; `cna-samples` relies on exactly that. There is one important
scope detail: the helper can copy only imported targets visible in the caller's
directory. CNA creates `SDL3`, `SDL3_image` and `SDL3_mixer` inside its own
subdirectory, so the template must re-run all three `find_package()` calls in
the parent scope before invoking the helper. Re-importing only SDL3 was proven
insufficient: `HelloGame.exe` imported `SDL3_image.dll` and `SDL3_mixer.dll`,
but neither was packaged. The template now re-imports and copies all three.

The old entry's *stated reason* ("functions defined via `include()` while CNA is
processed as our subdirectory are not visible back in this top-level scope") was
simply false, and it had been copied into `CMakeLists.txt` as a comment. Both are
now corrected.

---

## Not runtime-verified in this audit

Listed so the next session does not mistake silence for a passing result:

- The Android build was not executed: no Android SDK/NDK on this machine. CNA's
  own docs disagree with each other about its current state —
  `cna/docs/android-graphics-limitations.md:24-50` reports the NDK cross-compile
  failing inside sharp-runtime, while `cna/docs/devices-build.md:270-386` records
  an APK that built and ran on an emulator on 2026-07-05.
- Emscripten 4.0.7 compiled and linked complete `WEBGL2` and `CANVAS` bundles,
  but no browser was available to this Codex session, so page startup and frame
  rendering were not observed. CI's pinned Emscripten 6.0.3 was not available
  locally either.
- The MinGW `HelloGame.exe` and its PE dependency package were inspected but
  not executed on Windows or under Wine.
