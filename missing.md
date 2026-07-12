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

### Window visibly resizes 2-3 times at startup when `PreferredBackBufferWidth`/`Height` are set in the `Game` constructor

**Where:** interaction between `GraphicsDeviceManager` and the SDL_RENDERER
backend's window/renderer setup path (`cna/src/CNA/Internal/Backends/SdlRenderer/`).

**Problem:** Setting `graphics_.setPreferredBackBufferWidthProperty(800)` /
`setPreferredBackBufferHeightProperty(600)` in the `Game` subclass
constructor (before `Initialize()`), the idiomatic real-XNA pattern, causes
the window to be created at some default size, then visibly resized through
at least one intermediate size, before finally settling at the requested
size. Confirmed in SDL debug logs from an actual run:
```
[Renderer] FixedHeightDynamicWidth: outputSize=800x480 logicalSize=800x480
...
[Renderer] FixedHeightDynamicWidth: outputSize=800x480 logicalSize=640x480
...
[Renderer] FixedHeightDynamicWidth: outputSize=800x600 logicalSize=800x600
```
i.e. 800x480 → 640x480 → 800x600. This is not just a log artifact — the
project owner watched the actual window on screen while `HelloGame` ran and
confirmed it visibly flickers during this startup sequence ("to okno nabehne
ale blika").

**Workaround in cna-template:** `HelloGame`'s constructor does not call
`setPreferredBackBufferWidth/HeightProperty()` at all — it uses
`GraphicsDeviceManager`'s default resolution (matching `examples/demo_2d`'s
own constructor, which also never touches these properties) and reads the
actual size back at runtime in `Update()` instead of assuming a fixed one.
This avoids the resize cascade entirely.

**Suggested upstream fix:** `GraphicsDeviceManager`/the SDL_RENDERER backend
should apply the constructor-set `PreferredBackBufferWidth/Height` once,
directly, when first creating the window — not create a default-sized window
and then resize it (possibly more than once) during `Initialize()`. This is
standard real-XNA behavior (the window is created once, at the requested
size) and the current CNA behavior is a visible regression from it for any
consumer that sets a non-default resolution, which is an extremely common
thing for a game to do.

### `CNA_GRAPHICS_BACKEND` cache variable's `STRINGS` property omits `WEBGPU`, and WEBGPU backend target availability is checkout-dependent

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
