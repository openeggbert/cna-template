# NEXT — cna-template session log

## Done so far

- **Research**: read mobile-eggbert's CMakeLists.txt/README/CLAUDE.md/.gitignore/android
  in full; dispatched two research agents for CNA's own API (`README.md` §10 Usage
  Example, `examples/demo_2d/`, `examples/demo_devices/android/`) and for
  cna-samples' porting conventions (`CLAUDE.md`, `DEFERRED.md`, `tools/*.py`).
  Confirmed directly (not just from agent summaries) the exact `Game` subclass
  skeleton by reading `cna/README.md` §10 and `examples/demo_2d/src/*` myself.
- Confirmed `../cna` and `../cna_graphics` on this machine are two separate
  local checkouts of the *same* GitHub repo (`openeggbert/cna.git`) — not
  related repos. `cna-template` does not hardcode either name; see
  `CNA_ROOT_DIR` in `CMakeLists.txt`.
- Asked the project owner 3 judgment calls up front, all answered:
  license = **MIT**, Visual Studio deliverable = **CMake integration files**
  (CMakePresets.json + Visual Studio "Open Folder", not hand-authored
  .sln/.vcxproj), Android application ID = **org.openeggbert.cnatemplate**.
- `plan.md` written (English) — full context + task checklist.
- Git: first commit on `master` (`.gitignore` + `README.md` stub), pushed.
  `develop` branch created; all further work happens there.
- `CMakeLists.txt`: 5-backend selection block (SDL_RENDERER/EASYGL/BGFX/VULKAN/WEBGPU),
  modeled on mobile-eggbert's proven pattern, adapted for a configurable
  `CNA_ROOT_DIR` sibling path instead of a hardcoded directory name. Windows
  DLL copying uses CMake's `$<TARGET_RUNTIME_DLLS:>` generator expression
  instead of CNA's `cna_copy_sdl_runtime`/`cna_copy_mingw_runtime` helpers —
  those are defined via `include()` while CNA is processed as *our*
  subdirectory, so they are not visible back in cna-template's own top-level
  scope (would have been a latent bug on Windows, never actually exercised on
  this Linux dev machine — worth remembering if a future session sees a
  mobile-eggbert-style project fail this exact way on a real Windows build).
- `cmake/toolchains/mingw-w64.cmake` added (adapted from mobile-eggbert, no
  changes needed).
- `include/HelloGame/HelloGame.hpp` + `src/HelloGame/{HelloGame.cpp,Program.cpp}`:
  minimal interactive `Game` subclass — loads `Content/logo.png`, moves it with
  arrow keys, Escape quits. Uses `GetTypeNameHPP()`/`GetTypeNameCPP()`.
- `Content/logo.png`: original placeholder (128x128, generated with PIL —
  blue rounded badge, egg shape, "CNA" text). Not reused game art.
- **Actually built and ran HelloGame** (not just compiled — this caught two
  real bugs the source-reading phase missed):
  1. `GraphicsDevice::Clear(const Color&)` clears target+depth+stencil to
     match real XNA/FNA semantics; SDL_RENDERER (2D-only, no depth/stencil)
     throws `std::runtime_error` on that path. Fixed by using the
     `Clear(r,g,b,a)` float overload instead (same one CNA's own `demo_2d`
     example uses) — this is the *portable-across-backends* idiom and should
     be called out in the README's porting section, since the framework's
     own `README.md` §10 "Usage Example" uses the `Clear(Color)` overload
     that only works on 3D-capable backends (EASYGL/BGFX/VULKAN/WEBGPU), not
     SDL_RENDERER.
  2. Setting `PreferredBackBufferWidth/Height` in the constructor caused the
     window to visibly resize 2-3 times at startup — confirmed both in SDL
     debug logs (800x480 → 640x480 → 800x600) and by the project owner
     watching it happen live ("to okno nabehne ale blika" — the window
     flickers). Fixed by not setting a preferred size at all (matches
     `demo_2d`'s own approach) and reading the actual size back in `Update()`.
  Verified clean (builds + runs the full smoke-test duration with no
  exceptions, stable single resolution, correct backend capability logging)
  on both **SDL_RENDERER** (`SDL_VIDEODRIVER=dummy`) and **EASYGL**
  (`xvfb-run` + software Mesa GL). Did not test BGFX/VULKAN/WEBGPU locally
  (Vulkan needs real/virtual GPU support not attempted here; WEBGPU may not
  even be available in this particular `../cna` checkout — see below).

## Important discovery: WEBGPU backend availability is checkout-dependent

This machine's `../cna` checkout's own `CMakeLists.txt` only wires up 4
backend targets (`cna_backend_graphics_{sdl_renderer,easygl,bgfx,vulkan}`) —
no `cna_backend_graphics_webgpu`. The sibling checkout `../cna_graphics`
(same repo, newer commit) does advertise WEBGPU as an "experimental fifth
backend" in its README, and mobile-eggbert's CMakeLists.txt (which builds
against `../cna_graphics`, not `../cna`) references
`cna_backend_graphics_webgpu` defensively via `if(TARGET ...)`.
`cna-template`'s `CMakeLists.txt` follows the same defensive pattern (the
`if(TARGET cna_backend_graphics_webgpu)` check before linking), so it will
correctly pick up WEBGPU if/when the user's own `../cna` checkout has it, and
degrade gracefully (falls through to plain `CNA`/`SDL3::SDL3` link) if not.
Worth a one-line README callout: WEBGPU availability depends on how current
your CNA checkout is.

## In progress

- Android project scaffold (`android/`), modeled on `mobile-eggbert/android`
  and CNA's own `examples/demo_devices/android/...` sample. App ID
  `org.openeggbert.cnatemplate`. Not started yet this is the next concrete
  step.

## Not started yet

- Web (Emscripten) build path: CMakeLists.txt branch is written (Content
  preload via `--preload-file`) but not yet actually build-tested with
  `emcmake`/emsdk (need to check whether emsdk is available on this machine
  before attempting).
- `CMakePresets.json` with per-backend presets + a Windows/MSVC preset using
  the Visual Studio 17 2022 generator (the "Visual Studio files" deliverable,
  per the owner's explicit decision — CMake integration, not hand-written
  .sln/.vcxproj).
- Full `README.md` rewrite: build instructions per platform x backend,
  Android instructions, Web instructions, Visual Studio instructions, and the
  C#→CNA porting guide (property getter/setter mapping, GetTypeName
  requirement, XNB non-support + asset conversion tooling in cna-samples,
  fonts/.fx handling, and the SDL_RENDERER Clear() gotcha discovered above).
- `CLAUDE.md`.
- `LICENSE` file (MIT, decided; not yet written to disk).
- Final NEXT.md update + final push once everything above lands.

## Commits so far (develop branch, on top of the master root commit)

1. `docs: add project plan` (plan.md)
2. `build: add root CMakeLists.txt with 5-backend selection and MinGW toolchain`
3. `feat: add HelloGame example and placeholder Content`
4. `fix: HelloGame startup flicker and SDL_RENDERER crash`

Only `master` has been pushed so far. `develop` has not been pushed yet —
per the owner's instructions, push occasionally (every several commits or at
a milestone), not after every commit. Plan: push once Android + Web + VS
presets + README + CLAUDE.md are in place (i.e. once the "buildable template
for all 4 platforms" milestone is reached), and again at the very end.

## Recommended next step for a future session

Pick up at "Android project scaffold" above. Read CNA's
`examples/demo_devices/android/com.openeggbert.cna.demodevices/` directory
structure directly (already identified as a complete, working, copyable
Android scaffold by the CNA-API research agent this session) and adapt it +
mobile-eggbert's `android/` the same way `CMakeLists.txt` was adapted:
change the package/app ID to `org.openeggbert.cnatemplate`, point the CMake
`externalNativeBuild` path at this repo's own root `CMakeLists.txt`, keep the
SDL Java glue path pointing at `${CNA_ROOT_DIR}` (which in this repo's own
`android/app/build.gradle` should resolve via a relative path back up to
`../../cna/third_party/SDL/android-project/...`, mirroring mobile-eggbert's
`build.gradle` `java.srcDirs` entry — verify the relative path depth is
correct for `cna-template/android/app/` before assuming it matches
mobile-eggbert's exactly, since the nesting is the same but worth a sanity
check). Do not attempt to actually run a Gradle build (no Android
SDK/NDK confirmed available on this machine) — creating the files correctly
is the goal; the owner will build/test it.
