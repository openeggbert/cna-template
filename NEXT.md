# NEXT — cna-template session log

## Post-completion addition: missing.md

After the checklist below was finished and pushed, the project owner asked
for all the upstream problems found in CNA/sharp-runtime/mobile-eggbert
during this session to be written up in English. Added `missing.md` at the
repo root (own file, not folded into README, since it documents upstream
framework issues rather than cna-template usage): the SDL_RENDERER
`Clear(Color)` crash (and that CNA's own README §10 example uses the
non-portable overload), the startup resize/flicker bug, the
`cna_copy_sdl_runtime`/`cna_copy_mingw_runtime` CMake scoping issue,
sharp-runtime's missing MinGW-cross zlib dependency, the WEBGPU
checkout-dependent-availability gap, and a small stray-argument issue in
mobile-eggbert's own `CMakeLists.txt`. Committed and pushed to `develop`.

## Status: all `plan.md` checklist items complete

Every task in `plan.md` §10 is done. This session built a complete,
buildable cna-template from an empty repository: CMake scaffolding for all 5
CNA graphics backends, a working interactive `HelloGame` example (build- and
run-tested on 2 of the 5 backends), Android and Web build support, Visual
Studio CMake integration, and full documentation (README porting guide,
CLAUDE.md, plan.md, this file).

## What was done, in order

1. **Research**: read mobile-eggbert's `CMakeLists.txt`/`README.md`/`CLAUDE.md`/
   `.gitignore`/`android/` in full directly; dispatched two research agents for
   CNA's own API (`cna/README.md` §10 Usage Example, `examples/demo_2d/`,
   `examples/demo_devices/android/`) and cna-samples' porting conventions
   (`CLAUDE.md`, `DEFERRED.md`, `tools/*.py`); then verified the exact `Game`
   subclass skeleton myself by reading `cna/README.md` §10 and
   `examples/demo_2d/src/*` directly rather than trusting paraphrase alone.
2. Confirmed `../cna` and `../cna_graphics` on this machine are two separate
   local checkouts of the *same* GitHub repo (`openeggbert/cna.git`), with
   `../cna_graphics` a commit or so ahead (it has the WEBGPU backend wired
   up; `../cna` at the time of this session did not — see "WEBGPU
   availability" below). `cna-template` does not hardcode either directory
   name — see `CNA_ROOT_DIR` in `CMakeLists.txt`.
3. Asked the project owner 3 judgment calls up front (all answered before any
   implementation): license = **MIT**, Visual Studio deliverable = **CMake
   integration files** (not hand-authored `.sln`/`.vcxproj`), Android
   application ID = **org.openeggbert.cnatemplate**.
4. `plan.md` written — full English context + task checklist.
5. Git: first commit on `master` (`.gitignore` + `README.md` stub), pushed.
   `develop` branch created; all further work happened there, 9 commits,
   pushed at 2 milestones (see "Commits" below).
6. `CMakeLists.txt`: 5-backend selection block (SDL_RENDERER/EASYGL/BGFX/
   VULKAN/WEBGPU) modeled on mobile-eggbert's proven pattern, adapted for a
   configurable `CNA_ROOT_DIR`. Windows DLL copying uses CMake's own
   `$<TARGET_RUNTIME_DLLS:>` generator expression instead of CNA's
   `cna_copy_sdl_runtime`/`cna_copy_mingw_runtime` helpers, since those are
   defined via `include()` while CNA is processed as *our* subdirectory and
   are therefore not visible back in cna-template's own top-level scope (this
   would have been a latent bug on a real Windows build, copied uncritically
   from mobile-eggbert — worth remembering if a similar project is seen to
   fail this exact way).
7. `cmake/toolchains/mingw-w64.cmake` (adapted from mobile-eggbert, no
   changes needed).
8. `include/HelloGame/HelloGame.hpp` + `src/HelloGame/{HelloGame.cpp,Program.cpp}`:
   minimal interactive `Game` subclass — loads `Content/logo.png`, moves it
   with arrow keys, Escape quits.
9. `Content/logo.png`: original 128x128 placeholder generated with PIL (blue
   rounded badge, egg shape, "CNA" text) — not reused game art.
10. **Actually built and ran HelloGame** (not just compiled) against
    **SDL_RENDERER** and **EASYGL**, which caught two real bugs the
    source-reading phase alone missed:
    - `GraphicsDevice::Clear(const Color&)` clears target+depth+stencil to
      match real XNA/FNA semantics; the 2D-only SDL_RENDERER backend has no
      depth/stencil buffer and throws `std::runtime_error`. **CNA's own
      README §10 "Usage Example" itself uses the non-portable
      `Clear(Color)` overload** — worth flagging upstream at some point, but
      out of scope for this repo. Fixed in HelloGame by switching to the
      `Clear(r,g,b,a)` float overload (same one CNA's own `demo_2d` uses).
    - Setting `PreferredBackBufferWidth/Height` in the constructor caused
      the window to visibly resize 2-3 times at startup (800x480 → 640x480
      → 800x600 per SDL debug logs) — confirmed live by the project owner
      watching it happen ("to okno nabehne ale blika", i.e. the window
      flickers). Fixed by not setting an explicit size at all (matches
      `demo_2d`'s own approach) and reading the actual size back in
      `Update()` instead.
    Verified via `SDL_VIDEODRIVER=dummy` (SDL_RENDERER) and `xvfb-run` +
    software Mesa GL (EASYGL): both build cleanly and run the full
    smoke-test duration with no exceptions, stable single resolution, and
    correct backend capability logging. BGFX/VULKAN were not tested locally
    (no real/virtual GPU support attempted for either); WEBGPU's
    availability depends on the CNA checkout (see below).
11. **Android** (`android/`): modeled directly on `mobile-eggbert/android`.
    Gradle wrapper + top-level project files copied verbatim (generic, no
    game-specific content); app module adapted (package
    `org.openeggbert.cnatemplate`, `HelloGameActivity extends SDLActivity`,
    `externalNativeBuild` points at this repo's own root `CMakeLists.txt`).
    `assets/Content` is a symlink back to the repo-root `Content/` directory,
    same convention as mobile-eggbert. No keystore/`key.properties`
    committed (release builds fail with a clear message until the user
    creates one locally — matches mobile-eggbert's pattern, and this
    session's `.gitignore` correctly excludes both). **Not build-tested**:
    no Android SDK/NDK available in this environment.
12. **Web (Emscripten)**: actually configured and built end-to-end with the
    real emsdk installed on this machine (`~/emsdk`, activated via
    `source emsdk_env.sh`; note the Bash tool's shell state does not persist
    between tool calls, so `source` and the cmake/build commands using it
    must be in the *same* tool invocation). Produced
    `HelloGame.html/.js/.wasm/.data` successfully; confirmed via
    `grep`/`strings` on the output `.js` that `Content/logo.png` was
    correctly embedded in the preloaded virtual filesystem at
    `/Content/logo.png`. Did not attempt a headless browser/Node run (would
    need a WebGL-capable headless environment not readily available here) —
    build + asset-packaging validation was judged sufficient given the
    effort/value tradeoff.
13. `CMakePresets.json`: presets for all 5 backends + a `web` preset +
    `windows-vs2022` (Visual Studio 17 2022 generator, gated to Windows via a
    host condition — this is the "Visual Studio files" deliverable per the
    owner's decision). Verified `cmake --list-presets` correctly lists
    everything and hides `windows-vs2022` on Linux, and that the
    `sdl-renderer` preset actually configures successfully. **Not
    build-tested with real Visual Studio/MSBuild** (none available on this
    machine) — the owner will test this directly on Windows.
14. Full `README.md`: overview, prerequisites, build instructions per
    platform x backend (including the MinGW cross-compile command line),
    Visual Studio section, Web section, Android section (including release
    signing instructions), and the full C#→CNA porting guide (property
    getter/setter mapping table with example, `GetTypeName` requirement,
    namespace/type-mapping rules, the `Clear()` gotcha discovered in this
    session, the XNB-non-support asset-conversion table referencing
    `cna-samples/tools/*.py`, and a recommended porting workflow).
15. `LICENSE` (MIT, Robert Vokac, matching mobile-eggbert's own license
    style for original non-Microsoft-derived code).
16. `CLAUDE.md`: project guidelines for future Claude Code sessions in this
    repo — where CNA/sharp-runtime/cna-samples live, the property convention,
    the `GetTypeNameHPP`/`GetTypeNameCPP` requirement, and an explicit rule to
    test graphics code against SDL_RENDERER specifically (the most
    restrictive backend) given what this session found.
17. Attempted to also validate the MinGW-w64 cross-compile path (mingw
    toolchain + compilers are installed on this machine, and CNA already has
    a prebuilt Windows SDL3 at `../cna/.sdl-prebuilt-Windows-x86_64/install`).
    Configuration failed inside **sharp-runtime's own** `CMakeLists.txt`
    (`find_package(ZLIB)` fails under the MinGW cross-toolchain — no
    MinGW-target zlib package installed on this machine). This is an
    upstream CNA/sharp-runtime cross-compile dependency gap, not a bug in
    any cna-template file — cna-template's own `CMakeLists.txt` parsed and
    progressed correctly up to the point where it delegates into CNA's
    subdirectory. Not pursued further (installing MinGW zlib system packages
    is outside this repo's scope); noted here for whoever next tries the
    MinGW path.

## WEBGPU backend availability is checkout-dependent

This machine's `../cna` checkout's own `CMakeLists.txt` only wires up 4
backend targets (`cna_backend_graphics_{sdl_renderer,easygl,bgfx,vulkan}`) —
no `cna_backend_graphics_webgpu`. The sibling checkout `../cna_graphics`
(same repo, newer commit) does have it. `cna-template`'s `CMakeLists.txt`
follows mobile-eggbert's defensive pattern (`if(TARGET cna_backend_graphics_webgpu)`
before linking) so it degrades gracefully either way — documented in the
README's backend section.

## Commits (develop branch, on top of the master root commit)

1. `docs: add project plan`
2. `build: add root CMakeLists.txt with 5-backend selection and MinGW toolchain`
3. `feat: add HelloGame example and placeholder Content`
4. `fix: HelloGame startup flicker and SDL_RENDERER crash`
5. `docs: add NEXT.md session log`
6. `feat: add Android project scaffold`
7. `build: add CMakePresets.json with per-backend and Visual Studio presets`
8. `docs: write full README and add MIT LICENSE`
9. `docs: add CLAUDE.md project guidelines`

`master` was pushed after its single initial commit. `develop` was pushed
once after commit 6 (Android scaffold milestone) and will be pushed again
now that everything in `plan.md` is complete (final push for this session).

## Recommended next steps for a future session

Nothing in `plan.md` is outstanding, so there's no forced next task. If
picking this back up, reasonable directions (none started, all optional):

- Build-test the Android path for real (needs Android SDK/NDK + a device or
  emulator — none available in this environment).
- Build-test `windows-vs2022` for real on Windows (the owner said they'd do
  this personally).
- Try BGFX/VULKAN backends locally if this machine gets real/virtual GPU
  support beyond software Mesa GL.
- If a MinGW-target zlib becomes available, retry the MinGW cross-compile
  smoke test (`cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake
  -DCNA_WINDOWS_DEPENDENCIES_ROOT=../cna/.sdl-prebuilt-Windows-x86_64/install`)
  to also exercise the `WIN32`/`MINGW` branches of `CMakeLists.txt`
  (`$<TARGET_RUNTIME_DLLS:>` copy, static libgcc/libstdc++ linking), which
  were never actually executed this session (config failed upstream before
  reaching them).
- Consider reporting the `Clear(Color)` non-portability-to-SDL_RENDERER
  finding upstream to the CNA project (its own README §10 example uses the
  non-portable overload) — out of scope for cna-template itself to fix.
