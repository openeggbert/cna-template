# NEXT.md

## 1. Project summary

`cna-template` is a CMake starter template for building applications on top
of **CNA**, a C++ reimplementation of the XNA 4.0 game framework programming
model (comparable to MonoGame/FNA, but native C++), built on SDL3 with a
pluggable graphics backend layer. CNA itself, `sharp-runtime` (.NET BCL
reimplementation CNA depends on), and `easy-gl` (OpenGL backend dependency)
live in sibling repositories (`../cna`, `../sharp-runtime`, `../easy-gl`),
not as submodules.

**Main goal:** give someone starting a brand-new CNA application — or
porting an existing XNA 4.0 C# game — a working, buildable skeleton: CMake
wiring for every CNA graphics backend and platform, a minimal example game
(`HelloGame`) meant to be deleted/replaced, and a documented porting guide.

**Current phase:** past initial build-out (all of `plan.md`'s original
checklist is done). Now in an active polish/hardening phase: real bugs keep
surfacing by actually building and running the code (not just reading it),
both in this repo and upstream in CNA, and getting fixed and documented as
they're found (see `missing.md`).

**Important architectural decisions** (see §6 for detail):
- `CNA_ROOT_DIR` is a configurable CMake cache path, not a hardcoded sibling
  directory name.
- `CNA_GRAPHICS_BACKEND` (a single string) is the *only* supported
  backend-selection interface — CNA's parallel `CNA_BACKEND_*` boolean
  options are deliberately never touched by this repo.
- Visual Studio support is CMake integration files (`CMakePresets.json`),
  not a committed `.sln`/`.vcxproj`.
- Android assets are synced via a Gradle task, not a git symlink (symlinks
  break on Windows checkouts without `core.symlinks`).
- The template's own CTest registration avoids CTest's shared `BUILD_TESTING`
  variable on purpose (see §6).

## 2. Current status

**Build status** (as last actually verified in this session — see §7 for the
exact commands; nothing was rebuilt to write this file, per instructions):
- `SDL_RENDERER` and `EASYGL`: build cleanly and run correctly. Verified
  repeatedly this session, most recently right after the backend-selection
  simplification (commit `c1abb6c`).
- `BGFX`, `VULKAN`, `HEADLESS`, `SOFTWARE`: **configure-verified only** —
  `cmake --preset <name>` succeeds and CNA reports the correct backend, but
  none of these four have actually been compiled or run in this session.
- `WEBGPU`: works only if the checked-out `../cna` defines the
  `cna_backend_graphics_webgpu` target; availability is checkout-dependent by
  design (see `missing.md`).
- Web (Emscripten): built successfully earlier in this session, but a later
  attempt **failed** — inside `sharp-runtime`'s own `Process.cpp`
  (`-Werror` on an unused parameter, Emscripten/Clang-specific). This is
  outside `cna-template`'s own files; see §4.
- Android (Gradle/APK): never build-tested in this environment (no Android
  SDK/NDK installed here).
- Windows/Visual Studio: never build-tested in this environment (no
  Visual Studio/MSBuild installed here). The project owner said they'd
  verify this personally on Windows.

**Test status:** `HelloGameSmoke` (a CTest-registered smoke test — renders 3
frames via `--smoke-test` and exits) passes on both `SDL_RENDERER` (under
`SDL_VIDEODRIVER=dummy`) and `EASYGL` (under `xvfb-run`). Not registered on
Android/Emscripten/cross-compiled builds (by design).
`.github/workflows/ci.yml` exists and its steps were replicated and verified
locally, but **the workflow has not yet been observed actually running on
GitHub Actions infrastructure itself** (no push/PR run has been watched).

**What currently works:** `HelloGame` — a minimal interactive example
(arrow keys move a sprite loaded from `Content/logo.png`, Escape quits) —
builds, runs, and renders correctly with no flicker on `SDL_RENDERER` and
`EASYGL` on Linux. Screenshot at `docs/screenshot.png`.

**What does not work / is unverified:** BGFX/VULKAN/HEADLESS/SOFTWARE
backends beyond configure; Android APK build; Windows/Visual Studio build;
Web/Emscripten build (currently broken by an external, unrelated issue); the
CI workflow's real execution on GitHub's infrastructure.

## 3. Recent changes

Most recent commits on `develop` (23 commits ahead of `master`; `master` has
just the initial `.gitignore` + `README.md` stub):

- **Fixed a real per-frame flicker bug**: `HelloGame::Draw()` was calling
  `device.Present()` itself, *in addition to* the framework's
  `Game::EndDraw()`, which already presents once per frame (verified
  line-for-line against FNA's `Game.cs`/`GraphicsDeviceManager.cs`, and
  measured empirically via an `LD_PRELOAD` symbol-count shim: 6 presents for
  3 frames before the fix, 3 after, on both `SDL_RENDERER` and `EASYGL`).
  Root cause traced to CNA's own `README.md` §10 example, which itself calls
  `Present()` in `Draw()` — confirmed via the full XNA/MonoGame sample
  corpus that **0 of 2489 + 506** real samples do this. Documented as an
  open upstream docs bug in `missing.md`; fixed locally in `HelloGame.cpp`.
- **Simplified backend selection** (`CMakeLists.txt`, commit `c1abb6c`):
  removed a ~60-line block that force-set CNA's `CNA_BACKEND_*` booleans
  (redundant with the `CNA_GRAPHICS_BACKEND` string CNA actually keys off,
  and it hardcoded a 5-backend world that silently rejected the `HEADLESS`
  and `SOFTWARE` backends CNA added later). Backend→target-name mapping
  collapsed from a 12-line if/elseif chain to one `string(TOLOWER ...)`.
  `CMakeLists.txt` went from 248 to 211 lines; all 7 backends are now
  selectable. `CMakePresets.json`, `README.md`, `CLAUDE.md` updated to match.
- **Added CI** (`.github/workflows/ci.yml`, `dependencies.lock`): GitHub
  Actions building Linux (`sdl-renderer`/`easygl` matrix + `ctest` under
  `xvfb-run`), Windows (`windows-vs2022` preset, build only), Web
  (Emscripten), Android (assemble debug APK), pinned to specific sibling-repo
  commits.
- **Added `--smoke-test` mode** to `HelloGame` (renders 3 frames, exits) and
  registered it as CTest's `HelloGameSmoke`, gated by a template-owned
  `CNA_TEMPLATE_BUILD_TESTS` option (not CTest's shared `BUILD_TESTING` —
  see §6 for why).
- **Fixed Android asset fragility**: replaced a committed symlink
  (`android/app/src/main/assets/Content` → `../Content`) with a Gradle
  `Sync` task that copies `Content/` into a generated assets directory
  before each build — the symlink would check out as a broken plain-text
  file on Windows without `core.symlinks`/Developer Mode enabled.
- **Verified and documented 2 upstream CNA bugs now fixed**: `Clear(const
  Color&)` crashing on `SDL_RENDERER` (fixed in `../cna` `41b36c67`), and a
  double backend-reconfiguration pass at startup (fixed in `../cna`
  `e4b24168`). Both re-verified in this session by actually rebuilding and
  running against the fixed `../cna`.
- **Corrected a mistaken finding**: an earlier claim that
  `cna_copy_sdl_runtime()`/`cna_copy_mingw_runtime()` were unreachable due to
  CMake directory scoping was empirically wrong (`function()`/`macro()`
  definitions do propagate up through `add_subdirectory()`, unlike
  variables) — corrected in `missing.md` rather than left standing.
- **`README.md` substantially expanded** over several rounds: screenshot,
  quick start, project structure, "where this fits" (positioning vs.
  `cna-samples`/`mobile-eggbert`), a customization checklist ("Making this
  your own project"), a table of contents, a backend status table,
  troubleshooting section, and a "Known upstream issues" pointer to
  `missing.md`.

## 4. Current blocker / main problem

**No build-breaking blocker on the primary, CI-covered path** (`SDL_RENDERER`
and `EASYGL` both build, run, and pass their smoke test). The most important
open item is instead an **uncommitted, unexplained working-tree change**
that changes default behavior and has not been verified:

**Symptom:** `git status --short` in the repo root currently shows:
```
 M CMakeLists.txt
 M src/HelloGame/HelloGame.cpp
```
- `CMakeLists.txt`: the desktop-path default for `CNA_GRAPHICS_BACKEND` was
  changed from `"SDL_RENDERER"` to `"VULKAN"` (one line, inside the
  `else()` branch that runs when not `EMSCRIPTEN`/`ANDROID`). This is the
  **second time** a stray, uncommitted edit has changed this same default to
  `VULKAN` during this session (the first instance was found and reverted a
  few commits ago) — origin unconfirmed; it may be manual local
  experimentation by the project owner (who was observed running a debug
  build directly), or leftover from a concurrent/parallel session touching
  this same working tree (both have happened repeatedly this session on
  sibling repos `../cna` and `../sharp-runtime`).
- `src/HelloGame/HelloGame.cpp`: a bare `#include <iostream>` was added with
  no corresponding usage visible in the diff — looks like an incomplete or
  abandoned edit (e.g. a debug `std::cout` that was added and then removed,
  or never finished).

**Affected files:** `CMakeLists.txt`, `src/HelloGame/HelloGame.cpp`.

**Suspected cause:** manual local testing/experimentation in the working
tree, not yet committed or reverted. Not confirmed which.

**What has already been tried:** nothing yet for *this* occurrence — it was
only just discovered while writing this file (per instructions, no build or
revert was performed). The prior, similar VULKAN-default change earlier in
the session was reverted with `git checkout`-equivalent editing back to
`"SDL_RENDERER"` after confirming via `git diff` that it was inconsistent
with the surrounding code (the `CNA_BACKEND_*` booleans path and comments
still assumed `SDL_RENDERER`).

**Why this matters:** `VULKAN` has never been build-verified in this
environment this session (only configure-verified). If someone runs a bare
`cmake -S . -B build` right now with no explicit `-DCNA_GRAPHICS_BACKEND=`
flag and no preset, they will silently get `VULKAN` instead of the
documented/CI-tested `SDL_RENDERER` default. (Note: this does **not** affect
CI or the documented Quick Start, both of which pass `CNA_GRAPHICS_BACKEND`
explicitly via a preset or `-D` flag, which overrides the plain cache
default.)

## 5. Known bugs and limitations

- **Confirmed bug, fixed upstream + verified:** `GraphicsDevice::Clear(const
  Color&)` crashed on `SDL_RENDERER`. Fixed in `../cna` (`41b36c67`).
- **Confirmed bug, fixed upstream + verified:** double backend
  reconfiguration at startup (visible resize/flicker). Fixed in `../cna`
  (`e4b24168`).
- **Confirmed bug, fixed locally:** `HelloGame::Draw()` double-presenting
  every frame (see §3). Root cause is a docs bug in CNA's own `README.md`
  §10, still **open upstream** (not CNA's code — see `missing.md`).
- **Incomplete / needs verification:** `BGFX`, `VULKAN`, `HEADLESS`,
  `SOFTWARE` backends only configure-tested, never actually compiled or run
  in this session.
- **Needs verification:** `.github/workflows/ci.yml` has never been observed
  running for real on GitHub Actions (only locally replicated).
- **Confirmed bug, external/out of scope:** Web/Emscripten build currently
  fails inside `sharp-runtime`'s own `Process.cpp` (`-Werror` on an unused
  parameter under Emscripten/Clang). Worked earlier this session against the
  same `sharp-runtime` checkout — likely fallout from unrelated concurrent
  work in `../sharp-runtime` by another party, not a `cna-template` issue.
- **Known open, upstream (sharp-runtime):** MinGW-w64 cross-compilation
  fails in `sharp-runtime`'s own `CMakeLists.txt` (`find_package(ZLIB)` —
  no MinGW-target zlib vendored). Documented in `missing.md`; native MSVC
  builds are unaffected.
- **Unknown / needs decision:** the two uncommitted working-tree changes
  described in §4 — keep, fix, or revert?
- **Not verified:** Android APK build and Windows/Visual Studio build — no
  SDK/NDK or MSBuild available in this development environment. The project
  owner intends to verify Windows personally.
- **Suspected non-issue, low risk:** CI's `android` job passes
  `-PcnaRootDir="${GITHUB_WORKSPACE}/cna"` explicitly even though the
  sibling-checkout layout used by `actions/checkout` should already resolve
  `../cna` correctly by default — redundant but harmless; not verified
  end-to-end since the Android CI job itself hasn't been observed running.

## 6. Architecture notes

- **`CNA_ROOT_DIR`**: CMake cache `PATH`, default
  `${CMAKE_CURRENT_SOURCE_DIR}/../cna`, override with `-DCNA_ROOT_DIR=...`.
  Must point at a sibling CNA checkout (not a submodule — CNA/sharp-runtime/
  easy-gl are all plain sibling git checkouts by convention).
- **`CNA_GRAPHICS_BACKEND`**: the *only* backend-selection interface this
  template exposes. A single string, validated against a list
  (`SDL_RENDERER EASYGL BGFX VULKAN WEBGPU HEADLESS SOFTWARE`). **Do not**
  reintroduce code that force-sets CNA's parallel `CNA_BACKEND_*` boolean
  options — that block existed before and was deliberately removed
  (commit `c1abb6c`) as redundant and because it silently blocked backends
  CNA added later. CNA only consults those booleans if a consumer
  explicitly sets one; this template never does.
- **Backend target naming**: CNA names every backend target
  `cna_backend_graphics_<backend, lowercased>` — the mapping in
  `CMakeLists.txt` is one `string(TOLOWER ...)`, not a per-backend
  if/elseif chain. Don't reintroduce the chain.
- **`HelloGame`** (`include/HelloGame/`, `src/HelloGame/`): minimal `Game`
  subclass, meant to be deleted/replaced by template consumers. **Hard
  invariant: never call `GraphicsDevice::Present()` from a `Draw()`
  override** — `Game::EndDraw()` (framework-owned) already presents exactly
  once per frame, matching real XNA/FNA. This is documented in this repo's
  `README.md` porting guide and `CLAUDE.md`; don't let it regress.
- **`HelloGameSmoke` CTest registration**: deliberately uses its own
  `CNA_TEMPLATE_BUILD_TESTS` option and a direct `enable_testing()` call,
  **not** CTest's shared `BUILD_TESTING` cache variable. `BUILD_TESTING` is
  explicitly forced `OFF` before `add_subdirectory(CNA)` because `meta-gl`
  (pulled in transitively via `easy-gl`, `EASYGL` backend only) defines its
  own `option(BUILD_TESTING ... OFF)`, and if this template's `BUILD_TESTING`
  were `ON` instead, `easy-gl`'s and `meta-gl`'s own internal test suites
  leak into `cna-template`'s own `ctest` run as spurious, never-built
  failures (verified/reproduced this session). This is load-bearing —
  don't "simplify" the test registration back to plain `include(CTest)`
  without re-checking this doesn't reappear.
- **Android** (`android/`): `app/build.gradle`'s `externalNativeBuild`
  points at this repo's own root `CMakeLists.txt`. CNA's location is
  configurable via a `-PcnaRootDir=...` Gradle property (mirrors
  `CNA_ROOT_DIR`). `Content/` is synced into a generated assets directory
  via a Gradle `Sync` task at `preBuild` time — **not** a git symlink
  (symlinks silently break on Windows checkouts without
  `core.symlinks`/Developer Mode; this was a real bug found and fixed this
  session).
- **Visual Studio**: delivered via `CMakePresets.json`'s `windows-vs2022`
  preset (CMake's native "Open Folder" integration), not a committed
  `.sln`/`.vcxproj` — a deliberate decision confirmed with the project
  owner, since generated project files would go stale and can't be
  validated from this (Linux) development environment anyway.
- **`missing.md`**: a living document of upstream (CNA/sharp-runtime/
  mobile-eggbert) issues found while actually building and running this
  template — not a `cna-template` changelog. Entries must be evidence-based
  (built + run, not just read/reasoned about) — an earlier entry in this
  same file was wrong because it wasn't empirically tested, and had to be
  corrected later.
- **Compatibility boundary**: this template must stay buildable across
  evolving CNA/sharp-runtime/easy-gl checkouts. Backend availability
  (`WEBGPU`/`HEADLESS`/`SOFTWARE` in particular) is checkout-dependent by
  design — handled with a runtime `if(NOT TARGET ...)` `FATAL_ERROR` giving
  a clear message, not a hardcoded assumption about which backends exist.

## 7. Useful commands

Configure + build (any backend):
```bash
cmake --preset sdl-renderer   # or: easygl, bgfx, vulkan, webgpu, headless, software
cmake --build --preset sdl-renderer
```

Run:
```bash
./cmake-build-sdl-renderer/HelloGame
```

Test (note: SDL_RENDERER and GL-capable backends need different environments):
```bash
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ctest --test-dir cmake-build-sdl-renderer --output-on-failure
xvfb-run -a ctest --test-dir cmake-build-easygl --output-on-failure   # EASYGL/BGFX/VULKAN
```

Reproduce/inspect the current blocker (§4):
```bash
git status --short
git diff CMakeLists.txt src/HelloGame/HelloGame.cpp
```

Web (Emscripten) — currently broken, see §4:
```bash
source /path/to/emsdk/emsdk_env.sh
emcmake cmake --preset web
cmake --build --preset web
```

Android (untested in this environment):
```bash
cd android && ./gradlew assembleDebug
```

Windows/Visual Studio (untested in this environment):
```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-vs2022
```

No lint/format tooling is configured in this repo (none was requested or added).

## 8. Next smallest tasks

1. **Resolve the uncommitted working-tree changes.**
   Goal: get back to a clean, intentional state — either commit a
   deliberate reason for the `VULKAN` default + the `<iostream>` include, or
   revert both.
   Files: `CMakeLists.txt`, `src/HelloGame/HelloGame.cpp`.
   Verify: `git status --short` is clean (or the change is committed with a
   clear message); `cmake --preset sdl-renderer && cmake --build --preset
   sdl-renderer` still succeeds afterward.

2. **Actually build and run `HelloGame` on the `VULKAN` backend.**
   Goal: close a real verification gap — only configure-tested so far. A
   Vulkan SDK was confirmed present on this machine earlier in the session
   (`vulkaninfo` works).
   Files: none expected (verification task); update `README.md`'s backend
   status table if it changes from "Not built/run".
   Verify: `cmake --preset vulkan && cmake --build --preset vulkan`, then
   run the resulting binary (or `ctest` under `xvfb-run`).

3. **Re-attempt the Web/Emscripten build** to check whether the external
   `sharp-runtime` `Process.cpp` `-Werror` failure has cleared (it's outside
   this repo; may resolve on its own as that sibling repo changes).
   Files: none in `cna-template`.
   Verify: `source .../emsdk_env.sh && emcmake cmake --preset web && cmake
   --build --preset web`.

4. **Confirm the GitHub Actions CI workflow actually passes** on real
   GitHub infrastructure (push or open a PR, then check the Actions tab) —
   it has only been verified by replicating its steps locally so far.
   Files: `.github/workflows/ci.yml` (read-only check unless a real failure
   surfaces).
   Verify: green run in GitHub's Actions UI for all 4 jobs (or a clear,
   fixable failure reason if not).

5. **Attempt a `BGFX` build locally**, time/feasibility permitting — it
   fetches dependencies externally via `FetchContent`, so this may be slow
   or infeasible in a constrained environment; worth a bounded attempt.
   Files: none expected.
   Verify: `cmake --preset bgfx && cmake --build --preset bgfx`.

## 9. Do not do yet

- No further refactor of `CMakeLists.txt`'s backend-selection block beyond
  what already landed in `c1abb6c` — it was just simplified; don't add
  complexity back without a concrete, demonstrated reason.
- Do not reintroduce a block that force-sets CNA's `CNA_BACKEND_*` boolean
  options — it was deliberately removed as redundant and backend-list-
  limiting (see §6).
- Do not switch `HelloGameSmoke`'s test registration back to CTest's shared
  `BUILD_TESTING` variable — this was deliberately avoided to prevent
  `easy-gl`/`meta-gl`'s own test suites from leaking into this project's
  `ctest` run (see §6). If you don't understand why, re-read that section
  before touching this.
- Do not attempt to fix the `sharp-runtime` MinGW/zlib gap or the Web
  build's `Process.cpp` `-Werror` issue *inside this repo* — both are
  upstream/sibling-repo problems, out of scope for `cna-template` (tracked
  in `missing.md` instead).
- Do not edit `../cna` or `../sharp-runtime` source directly from a
  `cna-template` session — the project owner has explicitly asked for
  upstream issues to be documented in `missing.md`, not patched around
  in-place; separate sessions handle actual upstream fixes.
- No unrelated cleanup or mass rewrites of `README.md`/`CLAUDE.md`/
  `missing.md` — they've been iteratively refined over many rounds; keep
  further edits scoped and additive, not a rewrite.
- No new example/demo beyond `HelloGame` — the template is deliberately
  minimal (explicit convention, stated in `CLAUDE.md`).
- Do not merge `develop` into `master` without explicit instruction from the
  project owner.
- No new features or scope expansion until the uncommitted-changes question
  in §4 is resolved — start there.

## 10. Resume prompt

```
Read NEXT.md in full before doing anything else. Inspect only the files
needed for task 1 in "Next smallest tasks" (the uncommitted working-tree
changes in CMakeLists.txt and src/HelloGame/HelloGame.cpp) -- don't refactor
or touch anything else. Make one small, verified improvement: resolve that
one task cleanly (commit with a clear reason, or revert). Run the relevant
build/test command from the "Useful commands" section to confirm nothing
broke. Then update NEXT.md to reflect the new state before finishing.
```
