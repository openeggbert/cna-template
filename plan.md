# plan.md — architecture and history

## Current architecture (as of the 2026-08-11 modernization)

The design goal is that **CNA's renderer count can change again without this
template needing a rewrite.** The previous architecture assumed 7 fixed
"backends"; CNA grew to 46 "renderers" under a renamed variable, and every part
of the old template that hard-coded the 7 broke silently. This is the
architecture built to survive the next such change.

### One manifest, not N duplicated lists

`cmake/renderers.json` is the single place template-side renderer facts live:
platform applicability, display requirement, dependency, CI tier, notes.
Everything else is generated or validated against it:

```
cmake/renderers.json
        │
        ├── cmake/CnaRenderers.cmake ──► CMakeLists.txt (validation, drift check)
        │
        └── tools/gen_renderer_files.py ──┬──► CMakePresets.json (generated)
                                           ├──► docs/renderers.md (generated)
                                           └──► CI matrix (.github/workflows/ci.yml,
                                                via --ci-matrix)
```

The manifest does **not** own renderer *names* — CNA does, in
`cna/cmake/RendererSelection.cmake`. Both the CMake side
(`cna_template_canonical_renderers()`) and the Python side
(`canonical_from_cna()`) parse that file directly and diff it against the
manifest. A renderer CNA adds or removes shows up as a named `FATAL_ERROR` (at
configure time) or a `--check` failure (in CI) rather than as a silent gap.

### Selection is CNA's own variable, passed through unmodified

`CNA_GRAPHICS_RENDERER` is CNA's public cache variable. This template does not
rename it, does not re-declare its default, and does not maintain a competing
list of "supported" values — it validates the value CNA would receive anyway,
earlier, with a better error message. If you set nothing, CNA's own default
applies.

### No renderer-name branching in application code or CMake

`CMakeLists.txt` has zero `if(CNA_GRAPHICS_RENDERER STREQUAL ...)` branches for
behavior. Renderer-specific facts it needs (display requirement, web link
flags, whether the renderer is in the GL family) come from
`cna_template_renderer_field()` reading the manifest.

`HelloGame` similarly avoids `#ifdef CNA_RENDERER_*` or name checks. It queries
`GraphicsDevice::SupportsCapability(CNA::GraphicsCapability::X)` and probes
`GameWindow::GetNativeSdlWindowEXT()` for "do I have a window", which is
accurate on all 46 renderers today and stays accurate if CNA adds a 47th.

### The `CNA` target is trusted, not second-guessed

CNA's `modules/CMakeLists.txt` defines `CNA` as an INTERFACE library that
already links every framework module, the resolved renderer target, and the
public compile definitions. The old template manually resolved a renderer
target name (`cna_backend_graphics_<lowercase>`) and wrapped it in a linker
group. Both are gone: `target_link_libraries(${_app} PRIVATE CNA
SHARP_RUNTIME)` is the entire link line, matching the pattern CNA's own
examples use.

### Diagnostics teach, not just reject

An invalid renderer/platform combination doesn't just fail — the error names
what platforms the renderer *is* valid on, offers a cross-compile command if one
exists (`cmake --preset windows-<renderer>` / `web-<renderer>`), and lists
renderers that *would* work here. An unknown name gets a "did you mean"
suggestion, and `EASYGL` specifically gets a migration hint, since it is the
single most likely stale name someone pastes from old documentation (including
this template's own, before this pass).

## History

### 2026-07 — original template (7 backends)

Built against a CNA revision with 7 selectable "graphics backends"
(`SDL_RENDERER`, `EASYGL`, `BGFX`, `VULKAN`, `WEBGPU`, `HEADLESS`, `SOFTWARE`)
chosen via `CNA_GRAPHICS_BACKEND`, with target names derived by lowercasing.
Reasonable for the CNA that existed then. See earlier `NEXT.md`/`missing.md`
history (preserved in git log) for the incremental fixes made during that
period — the double-Present() flicker, the `Clear(Color)` crash, and others,
all now folded into `missing.md`'s "Fixed upstream" section.

### 2026-08-11 — 46-renderer modernization

CNA's `develop` branch had, by this point, absorbed a large parallel
integration ("reconcile parallel feature lanes", `cna@7a64362`) that landed
DirectX 1–12, Direct2D, GDI, Glide, ten renderer-agnostic middleware
integrations (bgfx, Vulkan, WebGPU, Magnum, Wicked, Sokol, Diligent, LLGL,
FNA3D, PortableGL), three browser-DOM renderers (Canvas, HTML DOM, SVG DOM),
and split the old monolithic "EasyGL" backend into five public GL-profile
names. The public selection variable was renamed to `CNA_GRAPHICS_RENDERER` in
the same window. None of it was reflected in this template, which meant the
template's build was completely non-functional against current CNA — proven
directly: the old `CNA_GRAPHICS_BACKEND=VULKAN` was silently ignored, and CNA
fell back to its own `OPENGLES3` default instead.

This pass:

- Established the canonical 46-renderer inventory directly from
  `cna/cmake/RendererSelection.cmake`, with per-renderer platform gates,
  dependencies and target names (five parallel research agents, one
  synthesized fact set — see the evidence ledger this session produced).
- Found and worked around two real upstream CNA bugs that block *any*
  downstream consumer, not just this template (`missing.md` CNA-1): a
  module-layout validator and a vendored-header include path both resolve
  against the consumer's `CMAKE_SOURCE_DIR` instead of CNA's own root.
- Found and worked around a third (CNA-2): the GL-profile compile definition
  does not propagate to a consumer, so a consumer's own code misreports which
  of the five GL renderers is active.
- Rebuilt the CMake, presets, CI, `HelloGame`, and every piece of documentation
  around the manifest-driven architecture described above.
- Re-verified every non-renderer claim the template made about CNA (XNB,
  models, fonts, effects, audio, input, networking) against current
  implementation rather than trusting old prose. The most significant reversal:
  CNA now has a complete `.xnb` read path (including LZX decompression); the
  template had twice stated CNA would never support this.

The next time CNA's renderer count changes, the intended procedure is the four
steps in `CLAUDE.md`'s "When CNA gains or loses a renderer" section — not
another audit of this scope.
