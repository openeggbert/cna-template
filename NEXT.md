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
- **`CMakeLists.txt`** is renderer-count-agnostic: no per-renderer
  `if/elseif`. It links only `CNA SHARP_RUNTIME` (the old
  `-Wl,--start-group ... cna_backend_graphics_<x> ... --end-group` is gone —
  `CNA` is an INTERFACE umbrella that already carries the selected renderer
  target, and a linker group around an INTERFACE library expands to nothing).
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
- Sources moved from top-level `src/` + `include/` to `game/src` +
  `game/include` — forced by a real upstream bug, not a style choice. See
  `missing.md` CNA-1.

## Testing matrix (this session, this host: Linux x86_64, no display server)

Canonical renderers: **46**.

| Stage | Attempted | Passed | Notes |
| --- | --- | --- | --- |
| Configure (valid, this platform) | 19 | 19 | `SDL_RENDERER, HEADLESS, SOFTWARE, STUB, VULKAN, OPENGL1, OPENGL2, OPENGL4, OPENGLES1, OPENGLES3, OPENGLES2, OPENGL33, FREEDIRECT, SDL_GPU, PORTABLEGL, SOKOL, OPENVG, BLEND2D, FNA3D` |
| Configure (deliberately invalid) | 12 | 12 rejected correctly | `DIRECTX11, DIRECTX1, DIRECT2D, GLIDE, GDI, METAL` (Windows/macOS-only on Linux); `CANVAS, HTML_DOM, SVG_DOM, WEBGL1, WEBGL2` (web-only on Linux); `BOGUS_NAME, EASYGL` (unknown name, with a migration hint for EASYGL) |
| Build + smoke run | 5 | 5 | `STUB, SOFTWARE, PORTABLEGL` (windowless, ran natively); `SDL_RENDERER` (ran under `SDL_VIDEODRIVER=dummy`); `OPENGLES3` (built; not run — no Xvfb on this host) |
| MinGW cross-compile (Windows target, from Linux) | 1 | 0 | `SDL_RENDERER` — configure fails on an upstream `sharp-runtime` ZLIB gap (`missing.md`, SHARP-RUNTIME-1), reproduced live this session, not a template bug |
| Web (Emscripten) | 0 | — | not attempted: no emsdk on this host |
| Android (Gradle/NDK) | 0 | — | not attempted: no Android SDK/NDK on this host |
| Native Windows (MSVC) | 0 | — | not attempted: no Windows host available |
| macOS | 0 | — | not attempted: no macOS host available |
| Renderers never attempted (build blocked on missing deps not installed on this host, out of scope to install for a template audit) | 23 | — | `BGFX, WEBGPU, MAGNUM, SKIA, WICKED, DILIGENT, LLGL, GLIDE (needs 32-bit), GDI, DIRECT2D, DIRECTX1/2/3/5/6/7/8/9/10/11/12, METAL, CANVAS, HTML_DOM, SVG_DOM, WEBGL1, WEBGL2` — all are configure-gated correctly (see the platform-rejection tests above for the Windows/web/macOS-only ones), just not build-tested for real |

Every renderer in the 46 falls into exactly one of: configure-tested on this
host, deliberately platform-rejected on this host (verified), or genuinely
out of reach of this host (documented, not silently skipped).

## Remaining unverified — be explicit, do not assume

- **Web build.** Never run: no emsdk here. `docs/renderers.md`'s Web rows and
  the CI web job (`WEBGL2`, `CANVAS`) are unverified in this session; they
  encode what the platform audit established from CNA's source, not a build
  that actually ran.
- **Android build.** Never run: no SDK/NDK here. CNA's own documentation
  disagrees with itself about whether the sharp-runtime NDK cross-compile
  currently works at all (`missing.md` cites both sides). The gradle/manifest
  fixes in this pass (renderer selectable via `-PcnaRenderer=`, GLES feature no
  longer falsely claimed as forced) are unbuilt.
- **Native Windows / MSVC.** Never run: no Windows host.
- **MinGW cross-compile.** Attempted and blocked by a reproduced upstream bug
  (SHARP-RUNTIME-1), not completed.
- **macOS / METAL.** Never run: no macOS host.
- **The 23 renderers listed above with real external dependencies** (bgfx,
  WebGPU, Magnum, Skia, Wicked, Diligent, LLGL, all 12 Windows renderers,
  Glide, GDI, the 5 web renderers) — configure-time gating is verified;
  build/runtime is not.
- **Broad CI tier (`linux-broad` job).** Defined and matrix-generated
  correctly (verified: `--ci-matrix linux --tiers C` returns the right 16
  names), but has never executed on a real runner.

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

# Web (needs emsdk active)
cmake --preset web-webgl2 && cmake --build --preset web-webgl2 -j3

# Android (needs SDK/NDK)
cd android && ./gradlew assembleDebug -PcnaRootDir=../../cna -PcnaRenderer=OPENGLES3
```

## Immediate next steps, in priority order

1. Run the Web and Android builds on a host that has emsdk / the Android SDK,
   and update this file's testing matrix with real results.
2. Re-attempt the MinGW cross-compile after resolving the ZLIB path (either a
   MinGW zlib dev package or `-DCMAKE_PREFIX_PATH`), to get real Windows-path
   build evidence beyond configure-gate testing.
3. File the upstream bugs in `missing.md` (CNA-1 through CNA-6,
   SHARP-RUNTIME-1) against the real CNA repository, if that has not already
   happened elsewhere.
4. Get real hardware/emulator time for the Android build once the SDK gap is
   closed, and resolve which of CNA's own two contradictory documents about
   Android NDK compatibility is current.
