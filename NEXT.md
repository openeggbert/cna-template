# NEXT.md — handoff

**Written 2026-08-11**, at the end of a full modernization pass driven by CNA
growing from 7 renderers ("backends") to 46. That growth had silently broken
every part of this template's build: the public CMake variable was renamed
(`CNA_GRAPHICS_BACKEND` → `CNA_GRAPHICS_RENDERER`), the target-naming scheme
changed (`cna_backend_graphics_<x>` → `cna_renderer_<x>`, no longer a uniform
lowercase of the renderer name), and `EASYGL` stopped being a selectable
renderer at all. Nothing in the template configured against current CNA.

This file is the accurate state as of that pass. Read `missing.md` for the full
upstream-bug ledger and `plan.md` for how the architecture is meant to keep
working the next time CNA's renderer count changes.

---

## Verified revisions

| Repo | Branch | SHA |
| --- | --- | --- |
| cna-template | next | pre-modernization: `1d86681fdb08b4324959fe0cd69fd687e65ee28e` |
| cna | develop | `7a64362efef4119bf880459ef1704fb2c52199e2` (== `origin/develop`, no divergence) |
| sharp-runtime | develop | `f827a6c5349234d5ac938886788ed8eca8fe1c10` |
| easy-gl | develop | `0b46d35c394a9fb6aea6a85c6587894b5013da33` |
| meta-gl | develop | `571d3a62fe166b9781ac6193d137b12ff3757620` |
| free-direct | develop | `934f72ff0c52902631fceeb52c006f3ed2767485` |

These are what `dependencies.lock` and `.github/workflows/ci.yml` pin. CNA's
canonical renderer count at this SHA is **46**, read directly from
`cna/cmake/RendererSelection.cmake:16` (the `set_property(CACHE
CNA_GRAPHICS_RENDERER PROPERTY STRINGS ...)` line) — not assumed from this
document.

## What changed and why

- **`cmake/renderers.json`** is now the one place template-side renderer
  metadata lives (platforms, display requirement, dependency, CI tier, notes).
  It does not own renderer *names* — CNA does.
- **`cmake/CnaRenderers.cmake`** parses CNA's canonical list at configure time
  (before `add_subdirectory`, so a bad name fails with a helpful message rather
  than CNA's bare `Unknown graphics renderer`), and again from the CMake cache
  after `add_subdirectory` (authoritative, catches drift the textual parse could
  miss). Both directions of drift — CNA has a renderer the manifest doesn't, or
  the reverse — are a `FATAL_ERROR` naming the exact difference.
- **`CMakeLists.txt`** is renderer-count-agnostic: no per-renderer application
  link branch. It links only `CNA` (the old
  `-Wl,--start-group ... cna_backend_graphics_<x> ... --end-group` is gone —
  `CNA` is an INTERFACE umbrella that already carries the selected renderer
  and Sharp Runtime component targets, and a linker group around an INTERFACE
  library expands to nothing). The one renderer-name check is a guarded,
  temporary compatibility patch for CNA's stale CANVAS method signature
  (`missing.md`, CNA-8), not application behavior.
- **`tools/gen_renderer_files.py`** generates `CMakePresets.json` and
  `docs/renderers.md` from the manifest, and doubles as the CI structure check
  (`--check`) and the CI matrix source (`--ci-matrix <platform> --tiers <b,c>`).
- **`game/src/HelloGame.cpp`** now queries
  `GraphicsDevice::SupportsCapability()` and probes
  `getWindowProperty().GetNativeSdlWindowEXT()` instead of assuming a window and
  3D exist. It heap-allocates the `Game` in `main()` because a stack-local one
  is unsafe under Emscripten's `-fwasm-exceptions` unwind-based main loop exit
  (documented upstream in `cna/docs/emscripten-mainloop-game-lifetime.md`).
- **`.github/workflows/ci.yml`** is tiered (structure / core / broad / platform)
  instead of one flat matrix, and the renderer lists in it come from the
  manifest via `--ci-matrix`, not from hand-typed YAML.
- **MinGW packaging was completed in the follow-up verification.** The
  application uses CNA's dynamic C++ runtime helper because static libstdc++
  fails on CNA's RTTI graph, and all three SDL packages are re-imported in the
  caller scope so `SDL3.dll`, `SDL3_image.dll` and `SDL3_mixer.dll` are copied.
- **The CI web pair was built locally with Emscripten 4.0.7.** Both `WEBGL2`
  and `CANVAS` produced HTML/JS/Wasm/data bundles; WEBGL2 carried its WebGL 2
  constraints and CANVAS correctly carried none. Upstream integration gaps
  found by those full builds are recorded in `missing.md` and narrowly bridged
  by the template. CNA's SDL prebuild cache is redirected into the template's
  ignored `build/` area, so a read-only CNA checkout now works and all web
  presets reuse the same Emscripten SDL artifacts.
- Sources moved from top-level `src/` + `include/` to `game/src` +
  `game/include` — forced by a real upstream bug, not a style choice. See
  `missing.md` CNA-1.

## Testing matrix (this session, this host: Linux x86_64, no display server)

Canonical renderers: **46**.

| Stage | Attempted | Passed | Notes |
| --- | --- | --- | --- |
| Configure (valid, this platform) | 19 | 19 | `SDL_RENDERER, HEADLESS, SOFTWARE, STUB, VULKAN, OPENGL1, OPENGL2, OPENGL4, OPENGLES1, OPENGLES3, OPENGLES2, OPENGL33, FREEDIRECT, SDL_GPU, PORTABLEGL, SOKOL, OPENVG, BLEND2D, FNA3D` |
| Configure (deliberately invalid) | 13 | 13 rejected correctly | `DIRECTX11, DIRECTX1, DIRECT2D, GLIDE, GDI, METAL` (Windows/macOS-only on Linux); `CANVAS, HTML_DOM, SVG_DOM, WEBGL1, WEBGL2` (web-only on Linux); `BOGUS_NAME, EASYGL` (unknown name, with a migration hint for EASYGL) |
| Native build (smoke where feasible) | 5 | 5 | `STUB, SOFTWARE, PORTABLEGL` (windowless, built and ran); `SDL_RENDERER` (built and ran under `SDL_VIDEODRIVER=dummy`); `OPENGLES3` (built; not run — no Xvfb on this host) |
| MinGW cross-compile (Windows target, from Linux) | 1 | 1 | `SDL_RENDERER` — configured without an external zlib prefix, compiled all 435 steps and linked `HelloGame.exe`; PE imports verified against the six packaged SDL/MinGW runtime DLLs; executable not run |
| Web (Emscripten 4.0.7) | 2 | 2 | `WEBGL2`, `CANVAS` — complete builds, all four deployable artifacts verified; browser runtime not run because no browser was attached to this session |
| Android (Gradle/NDK) | 0 | — | not attempted: no Android SDK/NDK on this host |
| Native Windows (MSVC) | 0 | — | not attempted: no Windows host available |
| macOS | 0 | — | not attempted: no macOS host available |
| Renderer builds still not attempted | 25 | — | `BGFX, WEBGPU, MAGNUM, SKIA, WICKED, DILIGENT, LLGL, GLIDE (needs 32-bit), GDI, DIRECT2D, DIRECTX1/2/3/5/6/7/8/9/10/11/12, METAL, HTML_DOM, SVG_DOM, WEBGL1` — all are configure-gated correctly; these need another dependency/platform or are outside the representative web pair |

Every renderer in the 46 is covered by the native configure audit, a deliberate
platform rejection, or a real cross-platform build; unbuilt combinations are
listed rather than implied to pass.

## Remaining unverified — be explicit, do not assume

- **Web runtime.** `WEBGL2` and `CANVAS` both compile and link, but the generated
  pages were not started in a browser because this Codex session exposed no
  browser instance. CI currently pins Emscripten 6.0.3 while the available
  local SDK was 4.0.7, so CI-version compatibility also awaits a real run.
- **Android build.** Never run: no SDK/NDK here. CNA's own documentation
  disagrees with itself about whether the sharp-runtime NDK cross-compile
  currently works at all (`missing.md` cites both sides). The gradle/manifest
  fixes in this pass (renderer selectable via `-PcnaRenderer=`, GLES feature no
  longer falsely claimed as forced) are unbuilt.
- **Native Windows / MSVC.** Never run: no Windows host.
- **MinGW runtime.** The full cross-build now passes, but `HelloGame.exe` was not
  run on Windows or under Wine. Native Windows behavior therefore remains
  unverified even though the PE import/package check is complete.
- **macOS / METAL.** Never run: no macOS host.
- **The 25 renderer builds listed above** (bgfx, WebGPU, Magnum, Skia, Wicked,
  Diligent, LLGL, the 12 DirectX/Direct2D selectors, Glide, GDI, Metal, and the
  3 remaining web renderers) — configure-time gating is verified;
  build/runtime is not.
- **Broad CI tier (`linux-broad` job).** Defined and matrix-generated
  correctly (verified: `--ci-matrix linux --tiers C` returns the right 16
  names), but has never executed on a real runner.

## CI repairs, 2026-08-13 — not yet confirmed by a run

The first CI run after the modernization pass was red on every native job. The
three causes were separate and none of them was a renderer problem:

1. **Every Linux job died inside SDL, before any renderer was compiled.**
   `SDL could not find X11 or Wayland development libraries`. The runner image
   carries the X11 runtime libraries but not their headers, and the workflow
   installed FFmpeg and Mesa but none of SDL's own build dependencies. It hit
   `HEADLESS`, `SOFTWARE` and `STUB` too, because CNA configures its vendored
   SDL3 whatever the renderer. Fixed by an `SDL_LINUX_PACKAGES` env list shared
   by the tier-B and tier-C jobs.
2. **Windows/MSVC never reached CNA:** `Generator Visual Studio 17 2022 could
   not find any instance of Visual Studio` — windows-latest no longer has VS
   2022. Fixed by dropping the `-G`/`-A` pin and letting CMake pick the VS the
   runner actually has.
3. **Android failed while evaluating `android/app/build.gradle`,** at
   `arraycopy: element type mismatch`: the CMake argument list holds Groovy
   GStrings, which `toArray(new String[0])` cannot store. This is a template
   bug, not the upstream NDK question the job's `continue-on-error` was there
   for. Fixed by converting the elements; the failing and fixed expressions
   were both checked against Groovy 3.0.24 outside Gradle.

**Round 2, after the first run of those fixes.** All three moved the failure
further along and each uncovered exactly one thing standing behind it:

1. Linux now detects X11, ALSA and PulseAudio, and stops one check later on
   `Couldn't find dependency package for XTEST` — `libxtst-dev` was missing from
   the list, and `libdbus-1-dev`, `libibus-1.0-dev` and `libegl-dev` were added
   with it so the remaining optional SDL checks stop degrading silently.
2. Windows found Visual Studio, configured and compiled all of SDL3, and then
   died in CNA's SDL *install* step: it builds `Debug/SDL3.dll` and installs
   `Release/SDL3.dll`. That is an upstream multi-config bug, now `missing.md`
   CNA-10; the job switched to Ninja, which is single-config, to get past it.
3. Android got past line 56 and failed at line 80 on `doFirst()` — the
   missing-keystore guard sat on a `BuildType`, which is not a Task, so a
   release-only concern was killing the evaluation of `assembleDebug`. It now
   hangs off `assembleRelease`/`bundleRelease`.

**Round 3 — the first green native jobs this template has ever had on CI.** All
five tier-B Linux renderers now configure, build, and pass *both* smoke tests
(`SDL_RENDERER`, `OPENGLES3`, `HEADLESS`, `SOFTWARE`, `STUB`; run
`31668462089`). That is also the first time the 3D demo from `4d0a2c8` has run
anywhere. Two jobs are still red:

- **Windows.** The Ninja switch did not help, and the log says why: CNA's SDL
  sub-configure is passed no `-G`, so it takes CMake's Windows default —
  Visual Studio — regardless of the parent generator, and fails on CNA-10
  exactly as before. The job now also exports `CMAKE_GENERATOR=Ninja`, which
  CMake applies to any invocation without `-G`, including that child. Unproven
  until the next run.
- **Android.** Evaluation is fixed and 27 tasks execute; it now dies inside the
  NDK CMake configure (`configureCMakeDebug[arm64-v8a]`, cmake exit 1). AGP
  reports only the exit code and buries CMake's own message above a ~150-line
  Java stack trace, out of reach of the log API's tail. A `if: failure()` step
  now dumps the `.cxx` logs so the next red run names the actual cause instead
  of hiding it. This is the point where CNA's contradictory Android
  documentation finally becomes testable.

Nothing here was compiled locally — this session had no CNA checkout.

## Still unbuilt since the 3D demo landed

`4d0a2c8` ("demo: showcase 2D and 3D renderer capabilities") added the
`BasicEffect` / `DrawUserPrimitives` / depth-state path *after* the verification
matrix above was recorded. Since then the only green builds anywhere are the two
web ones, so that 3D path has been compiled exactly once (`WEBGL2`, which does
compile it) and **has never run on any renderer**. The renderers that were
actually executed — `STUB`, `SOFTWARE`, `PORTABLEGL` — all report `2d3d`, so a
re-run exercises code that has never been exercised. Treat the "native build
(smoke where feasible)" row above as evidence about `2b858e9`, not about HEAD.

## Commands for the next session

```bash
# Regenerate/verify presets + docs after any renderers.json edit
python3 tools/gen_renderer_files.py
python3 tools/gen_renderer_files.py --check

# Reproduce the configure sweep
for r in SDL_RENDERER HEADLESS SOFTWARE STUB VULKAN OPENGL1 OPENGL2 OPENGL4 \
         OPENGLES1 OPENGLES3 OPENGLES2 OPENGL33 FREEDIRECT SDL_GPU \
         PORTABLEGL SOKOL OPENVG BLEND2D FNA3D; do
  cmake -S . -B build-probe -DCNA_GRAPHICS_RENDERER=$r >/dev/null \
    && echo "$r OK" || echo "$r FAIL"
  rm -rf build-probe
done

# Web (needs emsdk active; these are the two CI representatives)
cmake --preset web-webgl2 && cmake --build --preset web-webgl2 -j3
cmake --preset web-canvas && cmake --build --preset web-canvas -j3

# Android (needs SDK/NDK)
cd android && ./gradlew assembleDebug -PcnaRootDir=../../cna -PcnaRenderer=OPENGLES3

# MinGW from Linux
cmake -S . -B build-probe --toolchain cmake/toolchains/mingw-w64.cmake \
  -DCNA_GRAPHICS_RENDERER=SDL_RENDERER
cmake --build build-probe --parallel 3
```

## Immediate next steps, in priority order

1. Run the Android build on a host with the SDK/NDK and resolve which of CNA's
   two contradictory Android status documents is current.
2. Serve the two generated web bundles in a real browser, and also exercise the
   CI-pinned Emscripten 6.0.3 rather than only the local 4.0.7 SDK.
3. Run the cross-built MinGW `HelloGame.exe --smoke-test` on Windows (or in a
   controlled Wine prefix), then build at least one Windows-only renderer to
   extend the current SDL_RENDERER-only Windows-path evidence.
4. File the upstream bugs in `missing.md` (CNA-1 through CNA-9 and
   SHARP-RUNTIME-1/2) against the real repositories, if that has not already
   happened elsewhere.
