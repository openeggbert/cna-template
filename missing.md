# Missing / problems found in CNA, sharp-runtime, and related projects

This file documents upstream issues discovered while building cna-template
(not deviations from a ported C# original — cna-template is a new project,
not a port). Each entry says where it was found, what's wrong, how
cna-template works around it, and what an upstream fix would look like.
Written in English per project convention, even though these were found
during a Czech-language session.

---

## CNA (`../cna`)

### `GraphicsDevice::Clear(const Color&)` crashes on the SDL_RENDERER backend

**RESOLVED upstream in `../cna` (develop branch), commit `41b36c67`.**
`IGraphicsBackend::SupportsDepthStencil()` (default `true`, overridden
`false` on `SdlGraphicsBackend`) is now used by
`GraphicsDevice::Clear(ClearOptions, ...)` to mask `DepthBuffer`/`Stencil`
out of the request when the active target has no real depth/stencil
buffer, degrading to a color-only clear instead of forwarding to
`ClearColorDepthAndStencil()` (matches FNA's own
`dsFormat == DepthFormat.None` masking behavior). Verified end-to-end
against this exact repro: `HelloGame` now uses `device.Clear(Color::
CornflowerBlue)` directly (see `src/HelloGame/HelloGame.cpp`) and runs
cleanly under `SDL_VIDEODRIVER=dummy` with no crash. The rest of this
entry is kept for historical context.

**Where:** `cna/src/Microsoft/Xna/Framework/Graphics/GraphicsDevice.cpp`,
`Clear(const Color& color)` (single-argument overload).

**Problem:** To match real XNA/FNA semantics, this overload clears
target+depth+stencil together — it forwards to
`Clear(ClearOptions::Target | ClearOptions::DepthBuffer | ClearOptions::Stencil, color, ...)`,
which routes to `backend_->ClearColorDepthAndStencil(...)`. The SDL_RENDERER
backend is 2D-only (no depth/stencil buffer) and its implementation of that
method is `SdlGraphicsBackend::ClearColorDepthAndStencil(...) { ThrowNo3D("ClearColorDepthAndStencil"); }`
— it unconditionally throws `std::runtime_error`. Any code that calls
`device.Clear(someColor)` crashes immediately on SDL_RENDERER, and works
fine on EASYGL/BGFX/VULKAN (which do have depth/stencil buffers).

**Confirmed by actually building and running**, not just reading the source:
`HelloGame` initially used `device.Clear(Color::CornflowerBlue)` (copied
directly from CNA's own documented example, see below) and crashed on
startup with `terminate called after throwing an instance of
'std::runtime_error' what(): SDL_Renderer does not support 3D:
ClearColorDepthAndStencil` under `SDL_VIDEODRIVER=dummy`.

**CNA's own documentation recommends the broken call.** `cna/README.md` §10
"Usage Example" — the canonical minimal `Game` subclass skeleton meant for
new consumers — uses `device.Clear(CornflowerBlue);` (the `Color` overload).
Anyone following that example verbatim and building against SDL_RENDERER
(the default/mandatory backend on Android and Web) hits this crash.
`examples/demo_2d/src/Game1.cpp` avoids the bug, but only by accident: it
uses the `Clear(float r, float g, float b, float a)` overload instead,
without any comment explaining that this choice (vs. the README's own
`Clear(Color)`) is what makes it portable.

**Workaround in cna-template:** `HelloGame::Draw()` uses
`device.Clear(r, g, b, a)` (the float overload, which only clears the color
target) instead of `Clear(const Color&)`. Documented in `README.md`'s
porting guide and in `CLAUDE.md` as a rule to follow for any future graphics
code in this repo.

**Suggested upstream fix:** either (a) make `Clear(const Color&)` degrade
gracefully on 2D-only backends (clear color only, skip depth/stencil,
instead of throwing) since a silent no-op-on-the-parts-that-don't-exist is
closer to "this backend doesn't have a depth buffer" than a hard crash, or
at minimum (b) fix `README.md` §10 to use the portable float overload, and
add a line to the SDL_RENDERER capability note in the crash message pointing
at `Clear(r,g,b,a)` as the 2D-safe alternative.

### `Game` startup visibly flickers for any `Game` that owns a `GraphicsDeviceManager` — the backend gets fully reconfigured twice

**RESOLVED upstream in `../cna` (develop branch), commit `e4b24168`.**
`GraphicsDeviceManager(Game*)`'s constructor no longer calls
`ApplyChanges()` — `Game::DoInitialize()`'s own `CreateDevice()` call,
moments later, is now the single source of truth for first-time setup,
matching FNA's own explicit guidance on this exact anti-pattern (see the
fix's own commit message). Verified: `HelloGame` under
`SDL_VIDEODRIVER=dummy` now logs `SetPresentationMode:
FIXED_HEIGHT_DYNAMIC_WIDTH` exactly once during startup instead of
twice. The rest of this entry is kept for historical context.

**Where:** `cna/src/Microsoft/Xna/Framework/GraphicsDeviceManager.cpp` and
`cna/src/Microsoft/Xna/Framework/Game.cpp`, specifically the interaction
between these three call sites:

1. `GraphicsDeviceManager::GraphicsDeviceManager(Game* game)`
   (`GraphicsDeviceManager.cpp:59-73`) unconditionally calls `ApplyChanges()`
   at the end of the constructor.
2. `GraphicsDeviceManager::ApplyChanges()` (`GraphicsDeviceManager.cpp:196`):
   since the device isn't null (it's `Game`'s own, obtained via
   `game_->getGraphicsDeviceProperty()`) but also isn't *owned* by the
   manager, it takes the "reconfigure existing device" branch and calls
   `applyToExistingBackend(gdi)` at line 232 — reconfiguration **#1**.
3. Separately and unconditionally, `Game::DoInitialize()`
   (`Game.cpp:640-646`) does:
   ```cpp
   graphicsDeviceManager_ = Services_.GetService<IGraphicsDeviceManager>();
   if (graphicsDeviceManager_ != nullptr)
   {
       graphicsDeviceManager_->CreateDevice();
   }
   ```
   `GraphicsDeviceManager::CreateDevice()` (`GraphicsDeviceManager.cpp:249`)
   also ends by calling `applyToExistingBackend(gdi)` (line 299) —
   reconfiguration **#2**, completely redundant with #1 since nothing
   meaningful changed in between.

`applyToExistingBackend()` (`GraphicsDeviceManager.cpp:541`) calls
`graphicsDevice_->SetPresentationMode(...)` followed by
`graphicsDevice_->Reset(pp, adapter)` — a real, visible reconfiguration of
the actual OS window's presentation/viewport/virtual-resolution state, not a
cheap no-op. So any `Game` subclass that owns a `GraphicsDeviceManager`
member pays for **two full backend reconfiguration passes** during startup
instead of one.

**Confirmed by actually building and running, not just reading the
source**, and specifically by comparing two real programs against each
other:
- `HelloGame` (owns a `GraphicsDeviceManager graphics_(this)` member — the
  standard, XNA-idiomatic pattern, exactly as CNA's own `README.md` §10
  "Usage Example" recommends) logs `[Renderer] SetPresentationMode:
  FIXED_HEIGHT_DYNAMIC_WIDTH` **twice** during startup, with a full
  `applyLogicalPresentation()`/`SDL_SetRenderLogicalPresentation` pair
  around each occurrence.
- CNA's own `examples/demo_2d` — rebuilt and run directly from this session
  for comparison — logs `SetPresentationMode` **zero** times, because
  `Game1` never constructs a `GraphicsDeviceManager` at all (confirmed by
  `grep -n GraphicsDeviceManager examples/demo_2d/src/Game1.*` returning no
  matches). This means `demo_2d` simply never exercises the buggy code path
  — it isn't evidence the double-reconfiguration is fine, it's evidence that
  `demo_2d` is not representative of the standard `GraphicsDeviceManager`-owning
  pattern every real XNA game (and CNA's own README example) uses.

This double reconfiguration is a plausible root cause of a real, visible
startup flicker the project owner watched happen live while `HelloGame` ran
("to okno nabehne ale blika" / "stale to blika" — the window flickers /
still flickers). An earlier hypothesis in this same investigation — that
setting `PreferredBackBufferWidth/Height` in the constructor caused a
*different-sized* intermediate resize (800x480 → 640x480 → 800x600) — was a
real, separate, and separately-fixed symptom (see below), but removing that
did not fully eliminate the flicker report, which led to this deeper trace.

**Earlier, now-superseded finding (kept for the record):** setting
`graphics_.setPreferredBackBufferWidthProperty(800)` /
`setPreferredBackBufferHeightProperty(600)` in the constructor caused one of
the two `applyToExistingBackend()` passes above to run with different
preferred-size state than the other, producing a visible resize through an
intermediate size (`FixedHeightDynamicWidth: outputSize=800x480
logicalSize=800x480` → `.../640x480` → `.../800x600` in SDL debug logs)
before settling. Removing the explicit size (using
`GraphicsDeviceManager`'s default resolution instead, matching `demo_2d`'s
own constructor) made both reconfiguration passes agree on the same size,
eliminating the *different-size* flicker — but the *same-size*
double-reconfiguration described above still happens on every startup
regardless, since it's unconditional in `Game::DoInitialize()`.

**Not worked around further in cna-template:** removing the
`GraphicsDeviceManager graphics_` member entirely would dodge this bug, but
would make `HelloGame` non-idiomatic (every real XNA game has one; it's
required for `PreferredBackBufferWidth`, `IsFullScreen`,
`ToggleFullScreen()`, etc., none of which `demo_2d` demonstrates either).
Per the project owner's explicit direction, this is documented here instead
of patched around at the application level or fixed directly in `../cna`.

**Suggested upstream fix:** `Game::DoInitialize()` should not
unconditionally call `graphicsDeviceManager_->CreateDevice()` when a
`GraphicsDeviceManager` already exists and already applied its initial
configuration via its own constructor. Either skip `CreateDevice()` when the
manager's device is already up to date (e.g. track whether `ApplyChanges()`
already ran once with no preference changes since), or have
`GraphicsDeviceManager`'s constructor *not* eagerly call `ApplyChanges()`
and instead let `Game::DoInitialize()`'s `CreateDevice()` call be the single
source of truth for the first-time setup. Either way, a `Game` with a
`GraphicsDeviceManager` member should reconfigure the backend exactly once
during startup, not twice.

### `CNA_GRAPHICS_BACKEND` cache variable's `STRINGS` property omits `WEBGPU`, and WEBGPU backend target availability is checkout-dependent

**PARTIALLY ADDRESSED upstream in `../cna` (develop branch), commit `e4b24168`.**
Directly testing `-DCNA_GRAPHICS_BACKEND=WEBGPU` against current
`develop` shows CMake configure already stops with a clear
`message(FATAL_ERROR ...)` — this was not actually a silent fallthrough
on this checkout (the "downstream linking fails with undefined-target
errors instead of a clear message" framing below turned out not to
reproduce for CNA's own configure step, only a theoretical concern about
*other* downstream projects' own separate logic potentially getting out
of sync with CNA's own selected backend). The message is now WEBGPU-
specific and actionable ("known backend name, but not defined by this
checkout... check out a revision where WEBGPU support has merged").
`STRINGS` deliberately still omits `WEBGPU`, since this checkout
genuinely doesn't implement it yet (correctly reflecting checkout
capability, not a bug) — add it back once WEBGPU support actually merges
into `develop`. The rest of this entry is kept for historical context.

**Where:** `cna/CMakeLists.txt` line ~75:
`set_property(CACHE CNA_GRAPHICS_BACKEND PROPERTY STRINGS "SDL_RENDERER" "EASYGL" "BGFX" "VULKAN")`
— no `"WEBGPU"`. Also, the `../cna` checkout used for this session's testing
never assigns `BACKEND_TARGET` to `cna_backend_graphics_webgpu` anywhere in
its backend-selection block (only `sdl_renderer`/`easygl`/`bgfx`/`vulkan`
are wired up), even though `cna/README.md` documents WEBGPU as a real (if
experimental) 5th backend, and a separate local checkout of the exact same
repository (`../cna_graphics`, one commit ahead) does have the
`cna_backend_graphics_webgpu` target.

**Problem:** A consumer project (like cna-template, or mobile-eggbert) that
supports "all 5 backends" by forwarding `CNA_GRAPHICS_BACKEND=WEBGPU` to CNA
gets silently different behavior depending on exactly which commit of CNA
happens to be checked out — no target gets built, and downstream linking
against `cna_backend_graphics_webgpu` fails with undefined-target errors
rather than a clear "your CNA checkout doesn't have WEBGPU yet" message.

**Workaround in cna-template:** `CMakeLists.txt` uses the same defensive
`if(TARGET cna_backend_graphics_webgpu)` check mobile-eggbert uses before
linking, so it degrades to a plain `CNA`/`SDL3::SDL3` link (no backend
library) rather than a hard CMake configure error — but this still doesn't
help a user who explicitly asked for WEBGPU and silently doesn't get it.
Documented as a known caveat in `README.md`.

**Suggested upstream fix:** update the `set_property(... STRINGS ...)` list
to include `"WEBGPU"` once the backend is considered supported in a given
checkout, and have the CMake backend-selection block emit a clear
`FATAL_ERROR` (not a silent fallthrough) when `CNA_GRAPHICS_BACKEND=WEBGPU`
is requested but `cna_backend_graphics_webgpu` isn't actually defined by
that checkout.

### `cna_copy_sdl_runtime()` / `cna_copy_mingw_runtime()` are not usable by downstream consumers (CMake scoping)

**CORRECTED — this claim does not actually hold, verified empirically.**
The theoretical CMake-scoping argument below (child-scope `function()`
definitions not visible in the parent after `add_subdirectory()`) is
true for *variables*, but not for `function()`/`macro()` *definitions*:
once CMake processes a `function()` command anywhere during a run
(including deep inside a nested `add_subdirectory()`), that function
becomes callable for the rest of the configure, including back up in
ancestor directories — this was never actually verified against real
CMake behavior when this entry was first written (see the original
"has apparently never actually executed" note below). Verified directly
against the real `../cna` repo: a throwaway consumer project that does
only `add_subdirectory(${CNA_ROOT_DIR} CNA)` (nothing else — no extra
`include()`) can call `cna_copy_sdl_runtime(sometarget)` immediately
afterward without any "Unknown CMake command" error. mobile-eggbert's
own `CMakeLists.txt` already calls `add_subdirectory` before its
`if(WIN32)` block that calls `cna_copy_sdl_runtime()`/
`cna_copy_mingw_runtime()`, so the call ordering is already correct too
— there is no actual bug here for either CNA or mobile-eggbert to fix.
No upstream change made. The rest of this entry (the original,
un-verified reasoning) is kept for historical context, since it's a
useful reminder to verify CMake-scoping claims empirically rather than
by pure reasoning — even this project's own earlier "theoretical, never
actually executed" analysis turned out to be wrong.

**Where:** `cna/cmake/ThirdPartySDL.cmake`, `function(cna_copy_sdl_runtime
target_name)` / `function(cna_copy_mingw_runtime target_name)`, pulled in via
`include(cmake/ThirdPartySDL.cmake)` at `cna/CMakeLists.txt` line 29.

**Problem:** These are ordinary CMake `function()`s, defined while CNA's own
`CMakeLists.txt` is being processed. When a downstream project (like
mobile-eggbert) does `add_subdirectory(<path-to-cna> CNA)`, CNA's
`CMakeLists.txt` — including this `include()` — runs inside the **child**
directory scope that `add_subdirectory()` creates. CMake function/macro
definitions made in a child scope are not visible back in the parent scope
once `add_subdirectory()` returns (this is standard, documented CMake
directory-scoping behavior, the same reason a variable `set()` in a
subdirectory doesn't leak into the parent without `PARENT_SCOPE`). So a
downstream project's own top-level `CMakeLists.txt` — which is exactly where
mobile-eggbert calls `cna_copy_sdl_runtime(${_game_target})`, inside its own
`if(WIN32)` block — cannot actually see or call these functions. Calling
them would fail with `Unknown CMake command "cna_copy_sdl_runtime"`.

This was **not caught by mobile-eggbert's own testing**, because that
project's development and CI both happen on Linux; the `if(WIN32)` branch
that calls `cna_copy_sdl_runtime()` has apparently never actually executed.
It would fail the moment someone tried a real native Windows (MSVC) build or
even a MinGW cross-build with `WIN32` set by the toolchain file.

**Workaround in cna-template:** does not call these functions at all.
Instead, `CMakeLists.txt`'s `if(WIN32)` block uses CMake's own built-in
`$<TARGET_RUNTIME_DLLS:target>` generator expression (CMake ≥3.21, which CNA
already requires) to copy SDL3/SDL3_image/SDL3_mixer and any other imported
runtime DLL dependency next to the executable — a self-contained approach
that doesn't depend on any CNA-internal helper.

**Suggested upstream fix:** if these helpers are meant for downstream
consumers (their names and doc comments suggest they are), CNA should
either (a) export them via a proper CMake package config
(`find_package(CNA)` + an installed `.cmake` module, so `include()` happens
in the *consumer's* scope), or (b) apply them internally to CNA's own
example targets and stop suggesting consumers call them directly, or (c) at
minimum, add a note in CNA's own docs that these functions are only valid
from within CNA's own directory-scope tree, not from a project that
`add_subdirectory()`s CNA in. Also worth a CI job that actually exercises
`if(WIN32)` codepaths (e.g. a MinGW cross-compile smoke build) so this class
of bug gets caught without needing a real Windows machine.

---

## sharp-runtime (`../sharp-runtime`)

### `find_package(ZLIB)` fails when cross-compiling for Windows via MinGW-w64 (no vendored/bundled zlib)

**Where:** `sharp-runtime/CMakeLists.txt` line 5, `find_package(ZLIB)`.

**Problem:** Attempting a MinGW-w64 cross-build of cna-template
(`cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake ...`, the
same cross-compilation workflow documented in mobile-eggbert's own README)
fails during configure, inside sharp-runtime's own `CMakeLists.txt`:
```
CMake Error at .../FindPackageHandleStandardArgs.cmake:233 (message):
  Could NOT find ZLIB (missing: ZLIB_LIBRARY) (found version "1.3.1")
Call Stack (most recent call first):
  .../FindZLIB.cmake:202 (FIND_PACKAGE_HANDLE_STANDARD_ARGS)
  sharp-runtime/CMakeLists.txt:5 (find_package)
```
It finds a `ZLIB_INCLUDE_DIR`/version (1.3.1, the **host** Linux system
zlib's `zlib.h`, picked up because `CMAKE_FIND_ROOT_PATH_MODE_INCLUDE` is
`ONLY` but the version probe still leaks through) but no `ZLIB_LIBRARY` that
actually matches the MinGW `x86_64-w64-mingw32` target, since no
MinGW-w64-targeted zlib package was installed in this environment. Unlike
SDL3/SDL3_image/SDL3_mixer, which CNA vendors and cross-builds from source
itself (see `cna/cmake/ThirdPartySDL.cmake`), sharp-runtime has no equivalent
vendoring for zlib — it unconditionally requires a pre-existing target zlib
to already be discoverable on the host.

**Impact:** blocks the MinGW cross-compilation workflow that mobile-eggbert's
own `README.md` documents as supported, on any machine that doesn't happen
to already have a MinGW-targeted zlib installed system-wide.

**Not fixed in cna-template**, since this is entirely inside sharp-runtime's
own `CMakeLists.txt`, not something a downstream consumer's `CMakeLists.txt`
can reasonably patch around. Documented here and in `NEXT.md` instead;
cna-template's MinGW cross-compile path (`cmake/toolchains/mingw-w64.cmake`)
was written and syntax-checked but could not be build-verified end-to-end
because of this.

**Suggested upstream fix:** either vendor zlib the same way CNA vendors SDL3
(build it from source into a prebuilt root per target triplet, cached like
`.sdl-prebuilt-*`), or at least document the MinGW-cross-compile zlib
dependency explicitly (e.g. `sudo apt install zlib1g-dev:mingw-w64` or
similar, if such a package exists — none was found installed on this
machine, and none was readily available via `apt list`).

---

## mobile-eggbert (`../mobile-eggbert`) — used as this template's structural model

### Stray extra argument to `include_directories()`

**Where:** `mobile-eggbert/CMakeLists.txt`:
```cmake
set(CNA_GRAPHICS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cna_graphics")
add_subdirectory("${CNA_GRAPHICS_SOURCE_DIR}" CNA)
include_directories("${CNA_GRAPHICS_SOURCE_DIR}/include" CNA)
```
The `add_subdirectory(... CNA)` call's second argument is a legitimate
`binary_dir` name (where CNA's build artifacts go inside mobile-eggbert's
build tree). The `include_directories(... CNA)` call's second `CNA` argument
looks like a copy-paste leftover from the line above — `include_directories()`
takes only directory paths, and `CNA` is not a valid directory relative to
mobile-eggbert's source tree. It's silently harmless (CMake just adds a
nonexistent extra "include directory" that never resolves to anything used),
but it's very likely an unintended copy/paste artifact rather than
intentional.

**Not present in cna-template:** `CMakeLists.txt` doesn't call
`include_directories()` for CNA's headers at all — it relies on CNA's own
`target_include_directories(CNA PUBLIC ...)` declaration, which already
propagates the include path transitively to anything that links the `CNA`
target. This is both simpler and avoids the stray-argument question
entirely.

**Suggested upstream fix:** drop the stray `CNA` argument from that
`include_directories()` call in `mobile-eggbert/CMakeLists.txt` (or drop the
whole call, per the same reasoning used in cna-template, since it's likely
redundant given CNA's own `PUBLIC` include directory export).
