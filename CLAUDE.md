# cna-template — Claude Code Guidelines

## Project Overview

**cna-template** is a starter repository for building applications on **CNA**, a
C++ reimplementation of the XNA 4.0 programming model on SDL3 with a large,
pluggable renderer layer. It ships one minimal example (`HelloGame`) that exists
to be deleted and replaced.

Sibling repositories (all read-only from here):

```
../cna             the framework — the source of truth for everything below
../sharp-runtime   .NET base-class-library types CNA is built on (always required)
../easy-gl         backs the five GL-profile renderers; itself needs ../meta-gl
../free-direct     backs the FREEDIRECT renderer only
../cna-samples     official XNA 4.0 samples ported to CNA — best porting reference
```

For CNA's own internal coding rules see `../cna/CLAUDE.md`. This file covers
only what applies to code in *this* repository.

## The rule that matters most: CNA is the source of truth

CNA changes fast. This template has already been broken once by assuming a
renderer world that no longer existed — it was written for 7 "backends" selected
through `CNA_GRAPHICS_BACKEND`, and by the time anyone checked, CNA had 46
renderers selected through `CNA_GRAPHICS_RENDERER`, and *nothing* configured.

So: **never hard-code a list of renderer names anywhere in this repository, and
never trust a renderer fact stated in prose — including in this file.** Verify it
against `../cna` before relying on it.

Concretely:

- The canonical renderer list lives in **CNA**, in the
  `set_property(CACHE CNA_GRAPHICS_RENDERER PROPERTY STRINGS ...)` line of
  `../cna/cmake/RendererSelection.cmake`.
- `cmake/CnaRenderers.cmake` reads that line at configure time and cross-checks it
  against this template's manifest, then re-checks against the CMake cache after
  `add_subdirectory(CNA)`. A renderer added to or removed from CNA therefore
  fails the configure with a drift error naming the exact difference. Do not
  weaken those checks.
- Template-side metadata (platform applicability, display requirement, CI tier,
  dependency, notes) lives in **`cmake/renderers.json`** and nowhere else.
- `CMakePresets.json` and `docs/renderers.md` are **generated** from that
  manifest. Never edit them by hand.

## When CNA gains or loses a renderer

1. Configure any preset. The drift check tells you exactly which names differ.
2. Add or remove the entry in `cmake/renderers.json`, filling in every field.
   Derive `platforms` from the gates in `../cna/cmake/RendererSelection.cmake`
   (`FATAL_ERROR` = hard, `WARNING` = soft) and from what CNA's own docs claim to
   support — those are different things, and `experimental` exists for the gap.
3. Regenerate: `python3 tools/gen_renderer_files.py`
4. Verify: `python3 tools/gen_renderer_files.py --check` (this is what CI runs)

That is the whole procedure. If you find yourself editing a renderer name in more
than one file, stop — the design has been broken and should be repaired instead.

## Build and test

```bash
cmake --list-presets
cmake --preset headless && cmake --build --preset headless -j3
ctest --preset headless
```

Use `-DCNA_GRAPHICS_RENDERER=<NAME>` for any renderer without a preset. Every
renderer CNA accepts is selectable; presets exist only for the common ones.

Smoke tests carry a CTest label describing what they need from the environment:

```bash
ctest --test-dir <build> -L headless      # no display needed at all
xvfb-run -a ctest --test-dir <build> -L display
```

**Build discipline:** cap compilation at `-j3`, always reuse an existing build
directory, and never build under `/tmp` or the session scratchpad. See
`../CLAUDE.md` for why (real, measured SSD wear).

## Code rules

- Match CNA/FNA XNA API names and signatures exactly; do not invent variations.
- Properties are getter/setter pairs (`getFooProperty()`/`setFooProperty()`),
  never raw public fields.
- Every concrete `Game` subclass needs `GetTypeNameHPP()` in its header and
  `GetTypeNameCPP(ClassName, "ClassName")` at file scope in its `.cpp`.
- **Never call `GraphicsDevice::Present()` from `Game::Draw()`.** `Game::EndDraw()`
  already presents exactly once per frame, exactly as real XNA/FNA does. Calling
  it yourself presents twice; SDL treats the backbuffer as invalid after a
  present, so the second one shows undefined content and the window flickers
  every frame. CNA's own `README.md` §10 still shows this wrongly — see
  `missing.md`.
- **Prefer capabilities over renderer names.** Ask
  `GraphicsDevice::SupportsCapability(CNA::GraphicsCapability::X)` at runtime
  rather than testing which renderer is compiled in.
  `GetGraphicsRendererName()` is for display only. `#ifdef CNA_RENDERER_*` in
  application code is a design smell here.
- Keep `HelloGame` inside the subset every renderer implements: `Clear`,
  `SpriteBatch`, `Texture2D`, keyboard input. Anything beyond that (vertex/index
  buffers, depth state, custom effects) must be capability-gated, because the
  2D-only renderers throw on those calls.
- Remember that four renderers open **no window at all** (`HEADLESS`, `SOFTWARE`,
  `STUB`, `PORTABLEGL`). Code that assumes a window must probe
  `getWindowProperty().GetNativeSdlWindowEXT()`, not assume.
- Keep sources under `game/`. A top-level `src/` or `include/` currently breaks
  the build — an upstream CNA bug documented in `missing.md`, with a preflight
  check in `CMakeLists.txt` that explains it.
- Do not add features or abstractions beyond what is asked. This is a template;
  `HelloGame` must stay easy to delete.
- Comments explain *why*, never *what*.

## Upstream boundaries

`../cna`, `../sharp-runtime`, `../easy-gl`, `../meta-gl` and `../free-direct` are
**read-only**. When an upstream bug blocks the template:

1. Record it in `missing.md` with evidence and the concrete upstream fix.
2. If the template can compensate in its own files without hiding the failure
   (an inherited include path, a preflight check with a clear message), do that
   and comment it with a pointer to `missing.md`.
3. Never patch the upstream repository, and never swallow a configure, link or
   runtime failure to make something look like it works.

## Documentation rules

- `README.md` — how to use the template. Keep the renderer detail in
  `docs/renderers.md`; do not duplicate the table.
- `docs/renderers.md` — generated. Do not hand-edit.
- `missing.md` — upstream bugs only, each with a current status.
- `NEXT.md` — handoff state: verified SHAs, what was tested, what was not.
- `plan.md` — architecture and history.
- Never state a renderer count, a platform claim or a "CNA does not support X"
  claim without checking `../cna` first. Several such claims in this repository
  have already been proven wrong by later CNA development — including a
  confident, twice-repeated statement that CNA would never read `.xnb`, which it
  now does.
