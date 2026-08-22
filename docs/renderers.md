<!-- GENERATED FILE — do not edit by hand. -->
<!-- Source: cmake/renderers.json — regenerate with tools/gen_renderer_files.py -->

# CNA renderers

CNA currently exposes **50 renderers**. You pick one at configure time with a single cache variable:

```bash
cmake -S . -B build -DCNA_GRAPHICS_RENDERER=OPENGLES3
```

If you set nothing, CNA chooses for you: `WEBGL2` on the web, `OPENGLES3` on Linux, `SDL_RENDERER` everywhere else.

## How to read this

**Platforms** are where the renderer is supported. cna-template refuses, with an explanation, to configure a renderer for a platform not listed here. A few renderers additionally have *experimental* platforms — CNA permits them but does not support them, so the template warns instead of refusing.

**Display** says whether the renderer opens a window. The four that do not (`HEADLESS`, `SOFTWARE`, `STUB`, `PORTABLEGL`) need no X server, no GPU and no video driver, which makes them the right choice for CI and servers. Note that `SOFTWARE` and `PORTABLEGL` genuinely rasterize but present nowhere.

**Scope** is `2D` or `2D+3D`. On a 2D-only renderer the 3D calls (`VertexBuffer`, `DrawUserPrimitives`, depth state) throw. Query `GraphicsDevice::SupportsCapability()` at runtime rather than testing the renderer's name.

**Tested by cna-template** describes this template's CI, not CNA's support: *every PR* is built and smoke-run on each push, *broad matrix* is the scheduled job, *on demand* is manual, and *not on CI runners* means a GitHub runner cannot provide the platform or SDK.

## SDL

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `SDL_RENDERER` | Linux, Windows, macOS, Android (+ Web experimental) | 2D | window | none | every PR | Most portable renderer, and CNA's default off Linux. 2D only: no VertexBuffer/IndexBuffer/depth buffer. CNA documents it as untested on the web -- use WEBGL2 or CANVAS there. |
| `SDL_GPU` | Linux, Windows, macOS | 2D+3D | window | system libshaderc | broad matrix | SDL3's own GPU abstraction. Needs libshaderc installed (libshaderc-dev). |

## OpenGL family (shared EasyGL implementation)

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `OPENGLES3` | Linux, Windows, macOS, Android | 2D+3D | window | sibling ../easy-gl (which needs ../meta-gl) | every PR | CNA's default renderer on Linux. Internally EasyGL. |
| `OPENGLES2` | Linux, Windows, macOS, Android | 2D+3D | window | sibling ../easy-gl (which needs ../meta-gl) | broad matrix | Internally EasyGL, GLES 2.0 profile. Cannot target the web — use WEBGL1. |
| `OPENGL33` | Linux, Windows, macOS | 2D+3D | window | sibling ../easy-gl (which needs ../meta-gl) | broad matrix | Internally EasyGL, desktop GL 3.3 core profile. |
| `WEBGL2` | Web | 2D+3D | window | sibling ../easy-gl (which needs ../meta-gl) | broad matrix | CNA's default renderer under Emscripten. Internally EasyGL. |
| `WEBGL1` | Web | 2D+3D | window | sibling ../easy-gl (which needs ../meta-gl) | broad matrix | WebGL 1 profile. Forcing MAX_WEBGL_VERSION=2 globally would break this. |

## OpenGL family (native, no EasyGL)

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `OPENGL1` | Linux, Windows | 2D+3D | window | system OpenGL | broad matrix | Legacy fixed-function OpenGL 1.x. Desktop Linux/Windows only. |
| `OPENGL2` | Linux, Windows, macOS | 2D+3D | window | system OpenGL | broad matrix | Desktop OpenGL 2.1 compatibility profile, GLSL 1.10. No EasyGL. |
| `OPENGL4` | Linux, Windows, macOS | 2D+3D | window | system OpenGL | broad matrix | Real desktop OpenGL 4.x core profile, independent of EasyGL. |
| `OPENGLES1` | Linux, Windows, Android | 2D+3D | window | system GLESv1_CM library + headers | broad matrix | OpenGL ES 1.1 fixed-function. Needs a real GLESv1_CM system library. |

## Modern GPU APIs

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `VULKAN` | Linux, Windows (+ Android experimental) | 2D+3D | window | system Vulkan SDK (find_package(Vulkan REQUIRED)) | broad matrix | Real 3D via Vulkan. Builds on a CI runner but needs a real or software ICD to run. |
| `WEBGPU` | Linux, Windows, macOS | 2D+3D | window | wgpu-native binary release, auto-downloaded | broad matrix | Despite the name this is a NATIVE renderer; CNA rejects it under Emscripten. |
| `METAL` | macOS | 2D+3D | window | Apple Metal framework | not on CI runners | macOS only. iOS/tvOS explicitly unvalidated upstream. |

## CPU / diagnostic

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `HEADLESS` | Linux, Windows, macOS, Android (+ Web experimental) | 2D+3D | **none** | none | every PR | No GPU and no window at all. The right choice for CI and servers. |
| `SOFTWARE` | Linux, Windows, macOS, Android (+ Web experimental) | 2D+3D | **none** | none | every PR | Real CPU rasterizer. Creates no window and does not link SDL3, so Present() puts nothing on screen -- it is a compute/test renderer, not a display one. |
| `STUB` | Linux, Windows, macOS, Android (+ Web experimental) | 2D+3D | **none** | none | every PR | Deliberate no-op renderer. Draws nothing, touches no GPU or window. |
| `PORTABLEGL` | Linux, Windows, macOS | 2D+3D | **none** | PortableGL single header via FetchContent | broad matrix | CPU OpenGL 3.x-style pipeline. Links no SDL3 and needs no display at all. |
| `TINYGL` | Linux, Windows, macOS, Android | 2D+3D | **none** | C-Chads/tinygl via FetchContent | broad matrix | CPU fixed-function OpenGL 1.x subset. Links no SDL3 and needs no display at all. |

## 2D vector rasterizers

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `SKIA` | Linux, Windows, macOS | 2D | window | a Skia you built yourself (CNA_SKIA_ROOT + CNA_SKIA_BUILD_DIR) | on demand | Skia is never fetched: CNA fails configure unless you point it at your own build. |
| `BLEND2D` | Linux, Windows, macOS | 2D | window | Blend2D + asmjit via FetchContent | broad matrix | Blend2D CPU vector rasterizer, presented through an SDL texture. |
| `OPENVG` | Linux, Windows, macOS | 2D | window | ShivaVG via FetchContent + system OpenGL | broad matrix | OpenVG 1.1 vector graphics on a fixed-function desktop GL context. |
| `NANOVG` | Linux, Windows, macOS | 2D | window | NanoVG via FetchContent + system OpenGL | broad matrix | NanoVG vector graphics through its desktop OpenGL 2 backend. |

## Browser DOM

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `CANVAS` | Web | 2D | window | none | broad matrix | Browser 2D canvas context. Needs no WebGL at all. |
| `HTML_DOM` | Web | 2D | window | none | broad matrix | Draws sprites as pooled CSS-transformed <div> elements. No WebGL. |
| `SVG_DOM` | Web | 2D | window | none | broad matrix | Draws sprites as real <svg>/<image> elements. No WebGL. |
| `PIXIJS` | Web | 2D | window | pinned PixiJS v7 UMD build | broad matrix | PixiJS retained-mode WebGL scene graph for SpriteBatch and render targets. |

## Portable middleware

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `BGFX` | Linux, Windows, macOS | 2D+3D | window | bgfx via FetchContent (pinned, patched) | broad matrix | bgfx cross-platform renderer. Primarily tested on Linux upstream. |
| `MAGNUM` | Linux, Windows, macOS | 2D+3D | window | Corrade + Magnum (system, or via FetchContent) | broad matrix | Desktop GL through Magnum's typed wrappers. Cannot target the web. |
| `WICKED` | Linux, Windows | 2D+3D | window | Wicked Engine (CNA_WICKED_ROOT; auto-fetch is OFF by default) | on demand | Dispatches to Vulkan or D3D12. Configure fails unless you supply CNA_WICKED_ROOT. |
| `SOKOL` | Linux, Windows, macOS | 2D+3D | window | sokol single header via FetchContent + system OpenGL | broad matrix | Adds CNA_SOKOL_API (GLCORE default; only GLCORE is verified upstream). |
| `DILIGENT` | Linux, Windows, macOS | 2D+3D | window | DiligentCore via FetchContent | on demand | Diligent picks D3D11/D3D12/Vulkan/GL/Metal at runtime. |
| `LLGL` | Linux, Windows, macOS | 2D+3D | window | LLGL via FetchContent + system libshaderc | on demand | LLGL picks OpenGL or Vulkan at runtime. Needs libshaderc installed. |
| `IGL` | Linux | 2D+3D | window | facebook/igl v1.1.1 plus selected bootstrap dependencies | on demand | Meta IGL with CNA's verified Linux OpenGL/GLX and Vulkan backends. |
| `FNA3D` | Linux, Windows, macOS | 2D+3D | window | FNA3D + MojoShader via FetchContent; needs Python3 | broad matrix | The XNA-shaped C library FNA renders through. Picks SDL_GPU/D3D11/GL at runtime. |

## Legacy APIs

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `FREEDIRECT` | Linux, Windows, macOS | 2D+3D | window | sibling ../free-direct | broad matrix | DirectDraw reimplemented on SDL3, so unlike DIRECTX* it builds on Linux. |
| `GDI` | Windows | 2D | window | Win32 GDI | not on CI runners | Classic Win32 GDI, 2D only. |
| `GLIDE` | Windows | 2D+3D | window | external glide3x.dll at runtime; 32-bit only | not on CI runners | 3dfx Glide 3.x. Requires a 32-bit (i686) Windows toolchain. |

## DirectX

| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `DIRECT2D` | Windows | 2D | window | Direct2D 1.1 | not on CI runners | Direct2D 1.1, 2D only. |
| `DIRECTX11` | Windows | 2D+3D | window | d3d11.h/dxgi.h | not on CI runners | Direct3D 11. |
| `DIRECTX12` | Windows | 2D+3D | window | d3d12.h/dxgi.h | not on CI runners | Direct3D 12. |
| `DIRECTX10` | Windows | 2D+3D | window | d3d10 (delivered via Wine + DXVK d3d10core when cross-testing) | not on CI runners | Direct3D 10. Shader-only pipeline: every draw needs real HLSL vs_4_0/ps_4_0. |
| `DIRECTX9` | Windows | 2D+3D | window | d3d9.h | not on CI runners | Direct3D 9. |
| `DIRECTX8` | Windows | 2D+3D | window | DXVK d3d8 import library via CNA_DX8_DXVK_LIB (mingw-w64 ships none) | not on CI runners | Direct3D 8, fixed-function only. DirectDraw no longer exists at this era. |
| `DIRECTX7` | Windows | 2D+3D | window | ddraw.h (IDirectDraw7) + d3d.h (IDirect3D7) | not on CI runners | DirectDraw 7 + Direct3D 7. Viewport objects removed at this era. |
| `DIRECTX6` | Windows | 2D+3D | window | ddraw.h (IDirectDraw4) + d3d.h (IDirect3D3) | not on CI runners | Same interfaces as DIRECTX5; adds real stencil buffer operations. |
| `DIRECTX5` | Windows | 2D+3D | window | ddraw.h (IDirectDraw4) + d3d.h (IDirect3D3) | not on CI runners | First era with no execute buffers: FVF DrawPrimitive only. |
| `DIRECTX3` | Windows | 2D+3D | window | ddraw.h (IDirectDraw2) + d3d.h (IDirect3D2) | not on CI runners | Real DirectX 3. Not to be confused with FREEDIRECT, which once held this name. |
| `DIRECTX2` | Windows | 2D+3D | window | ddraw.h (IDirectDraw v1) + d3d.h (IDirect3D2) | not on CI runners | DirectDraw v1 + Direct3D v2 DrawPrimitive. |
| `DIRECTX1` | Windows | 2D | window | ddraw.h (IDirectDraw v1 only) | not on CI runners | Real DirectDraw v1. No Direct3D at this era. |

## Presets

Renderers in common use have a ready-made preset; the rest are selected with `-DCNA_GRAPHICS_RENDERER=<NAME>` against any preset or a plain build directory. Every one of the 50 renderers above is selectable either way.

```bash
cmake --list-presets                 # what is available
cmake --preset opengles3             # configure
cmake --build --preset opengles3     # build
ctest --preset opengles3             # smoke test
```

<sub>Cross-checked against CNA's own canonical list of 50 renderers at generation time.</sub>
