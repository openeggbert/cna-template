#!/usr/bin/env python3
"""Generate CMakePresets.json and docs/renderers.md from cmake/renderers.json.

cna-template deliberately keeps one renderer manifest and derives everything
else from it, so a renderer cannot be listed correctly in one place and wrongly
in another.

    python3 tools/gen_renderer_files.py            # regenerate
    python3 tools/gen_renderer_files.py --check     # fail if anything is stale

--check is what CI runs. It regenerates into memory and compares, so a manifest
edit that was not followed by a regeneration is caught as a diff rather than
silently shipping inconsistent presets.

The manifest does not own the renderer *names*: CNA does. This script also
cross-checks the manifest against CNA's own canonical list when a CNA checkout
is reachable, using the same line cna/cmake/RendererSelection.cmake declares.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
MANIFEST = REPO / "cmake" / "renderers.json"
PRESETS = REPO / "CMakePresets.json"
RENDERER_DOC = REPO / "docs" / "renderers.md"

# Where CNA's canonical list lives, relative to a CNA checkout.
CNA_RENDERER_FILE = Path("cmake") / "RendererSelection.cmake"
CNA_STRINGS_RE = re.compile(
    r'set_property\(CACHE CNA_GRAPHICS_RENDERER PROPERTY STRINGS((?:\s+"[A-Z0-9_]+")+)\)'
)

CATEGORY_TITLES = {
    "sdl": "SDL",
    "gl-easygl": "OpenGL family (shared EasyGL implementation)",
    "gl-native": "OpenGL family (native, no EasyGL)",
    "modern-gpu": "Modern GPU APIs",
    "cpu": "CPU / diagnostic",
    "raster-2d": "2D vector rasterizers",
    "web-dom": "Browser DOM",
    "middleware": "Portable middleware",
    "legacy-api": "Legacy APIs",
    "directx": "DirectX",
}

PLATFORM_TITLES = {
    "linux": "Linux",
    "windows": "Windows",
    "macos": "macOS",
    "web": "Web",
    "android": "Android",
}

TIER_TEXT = {
    "B": "every PR",
    "C": "broad matrix",
    "D": "on demand",
    "X": "not on CI runners",
}


def load_manifest() -> list[dict]:
    data = json.loads(MANIFEST.read_text())
    return data["renderers"]


def canonical_from_cna() -> list[str] | None:
    """CNA's own renderer list, if a checkout is reachable."""
    for candidate in (REPO.parent / "cna", REPO / ".." / "cna"):
        path = (candidate / CNA_RENDERER_FILE).resolve()
        if path.exists():
            match = CNA_STRINGS_RE.search(path.read_text())
            if match:
                return re.findall(r'"([A-Z0-9_]+)"', match.group(1))
    return None


def slug(name: str) -> str:
    return name.lower().replace("_", "-")


def check_against_cna(renderers: list[dict]) -> list[str]:
    """Return human-readable drift problems (empty when in sync)."""
    canonical = canonical_from_cna()
    if canonical is None:
        return ["note: no CNA checkout found, renderer names not cross-checked"]

    ours = [r["name"] for r in renderers]
    missing = [n for n in canonical if n not in ours]
    extra = [n for n in ours if n not in canonical]

    problems = []
    if missing:
        problems.append(
            f"{len(missing)} renderer(s) in CNA but missing from the manifest: "
            + ", ".join(missing)
        )
    if extra:
        problems.append(
            f"{len(extra)} renderer(s) in the manifest but not in CNA: " + ", ".join(extra)
        )
    if not problems:
        problems.append(f"ok: manifest matches CNA's {len(canonical)} canonical renderers")
    return problems


# --------------------------------------------------------------------------
# CMakePresets.json
# --------------------------------------------------------------------------
def build_presets(renderers: list[dict]) -> dict:
    configure = [
        {
            "name": "base",
            "hidden": True,
            "cacheVariables": {"CMAKE_BUILD_TYPE": "Debug"},
        },
        {
            "name": "release",
            "hidden": True,
            "cacheVariables": {"CMAKE_BUILD_TYPE": "Release"},
        },
        {
            "name": "web-base",
            "hidden": True,
            "inherits": "release",
            # Lets `cmake --preset web-*` work directly instead of via emcmake.
            "toolchainFile": "$env{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake",
        },
        {
            "name": "mingw-base",
            "hidden": True,
            "inherits": "base",
            "toolchainFile": "${sourceDir}/cmake/toolchains/mingw-w64.cmake",
            "condition": {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Linux",
            },
        },
    ]
    build = []
    test = []

    for renderer in renderers:
        if not renderer.get("preset"):
            continue

        name = renderer["name"]
        platforms = renderer["platforms"]
        preset_name = slug(name)
        inherits = "base"
        condition = None

        # A renderer that can only ever target one platform gets a preset shaped
        # for that platform, so the preset is usable rather than merely present.
        if platforms == ["web"]:
            preset_name = f"web-{slug(name)}"
            inherits = "web-base"
        elif platforms == ["windows"]:
            preset_name = f"windows-{slug(name)}"
            inherits = "mingw-base"
        elif platforms == ["macos"]:
            condition = {
                "type": "equals",
                "lhs": "${hostSystemName}",
                "rhs": "Darwin",
            }

        entry = {
            "name": preset_name,
            # First sentence of the note, split on ". " so "GLES 2.0" survives.
            "displayName": f"{name} — {renderer['notes'].split('. ')[0].rstrip('.')}",
            "description": (
                f"{renderer['dimension'].upper()} renderer. "
                f"Platforms: {', '.join(platforms)}. "
                f"Dependency: {renderer['dependency']}."
            ),
            "inherits": inherits,
            "binaryDir": "${sourceDir}/build-" + preset_name,
            "cacheVariables": {"CNA_GRAPHICS_RENDERER": name},
        }
        if condition:
            entry["condition"] = condition

        configure.append(entry)
        build.append({"name": preset_name, "configurePreset": preset_name})

        # Only renderers that actually run on the host get a test preset.
        if platforms != ["web"]:
            test.append(
                {
                    "name": preset_name,
                    "configurePreset": preset_name,
                    "output": {"outputOnFailure": True},
                    "filter": {
                        "include": {
                            "label": "headless"
                            if renderer["display"] == "none"
                            else "display"
                        }
                    },
                }
            )

    return {
        "version": 3,
        "cmakeMinimumRequired": {"major": 3, "minor": 23, "patch": 0},
        "configurePresets": configure,
        "buildPresets": build,
        "testPresets": test,
    }


# --------------------------------------------------------------------------
# docs/renderers.md
# --------------------------------------------------------------------------
def build_doc(renderers: list[dict]) -> str:
    canonical = canonical_from_cna()
    count = len(renderers)

    out: list[str] = []
    add = out.append

    add("<!-- GENERATED FILE — do not edit by hand. -->")
    add("<!-- Source: cmake/renderers.json — regenerate with tools/gen_renderer_files.py -->")
    add("")
    add("# CNA renderers")
    add("")
    add(
        f"CNA currently exposes **{count} renderers**. You pick one at configure time "
        "with a single cache variable:"
    )
    add("")
    add("```bash")
    add("cmake -S . -B build -DCNA_GRAPHICS_RENDERER=OPENGLES3")
    add("```")
    add("")
    add(
        "If you set nothing, CNA chooses for you: `WEBGL2` on the web, `OPENGLES3` on "
        "Linux, `SDL_RENDERER` everywhere else."
    )
    add("")
    add("## How to read this")
    add("")
    add(
        "**Platforms** are where the renderer is supported. cna-template refuses, with an "
        "explanation, to configure a renderer for a platform not listed here. A few "
        "renderers additionally have *experimental* platforms — CNA permits them but does "
        "not support them, so the template warns instead of refusing."
    )
    add("")
    add(
        "**Display** says whether the renderer opens a window. The four that do not "
        "(`HEADLESS`, `SOFTWARE`, `STUB`, `PORTABLEGL`) need no X server, no GPU and no "
        "video driver, which makes them the right choice for CI and servers. Note that "
        "`SOFTWARE` and `PORTABLEGL` genuinely rasterize but present nowhere."
    )
    add("")
    add(
        "**Scope** is `2D` or `2D+3D`. On a 2D-only renderer the 3D calls "
        "(`VertexBuffer`, `DrawUserPrimitives`, depth state) throw. Query "
        "`GraphicsDevice::SupportsCapability()` at runtime rather than testing the "
        "renderer's name."
    )
    add("")
    add(
        "**Tested by cna-template** describes this template's CI, not CNA's support: "
        "*every PR* is built and smoke-run on each push, *broad matrix* is the scheduled "
        "job, *on demand* is manual, and *not on CI runners* means a GitHub runner cannot "
        "provide the platform or SDK."
    )
    add("")

    by_category: dict[str, list[dict]] = {}
    for renderer in renderers:
        by_category.setdefault(renderer["category"], []).append(renderer)

    for category, title in CATEGORY_TITLES.items():
        group = by_category.get(category)
        if not group:
            continue
        add(f"## {title}")
        add("")
        add("| Renderer | Platforms | Scope | Display | Dependency | Tested | Notes |")
        add("| --- | --- | --- | --- | --- | --- | --- |")
        for renderer in group:
            platforms = ", ".join(PLATFORM_TITLES[p] for p in renderer["platforms"])
            experimental = renderer.get("experimental") or []
            if experimental:
                platforms += " (+ " + ", ".join(
                    PLATFORM_TITLES[p] for p in experimental
                ) + " experimental)"
            scope = "2D+3D" if renderer["dimension"] == "2d3d" else "2D"
            display = "window" if renderer["display"] == "window" else "**none**"
            tested = TIER_TEXT.get(renderer["tier"], renderer["tier"])
            preset = f"`{slug(renderer['name'])}`" if renderer.get("preset") else "—"
            add(
                f"| `{renderer['name']}` | {platforms} | {scope} | {display} | "
                f"{renderer['dependency']} | {tested} | {renderer['notes']} |"
            )
            del preset
        add("")

    add("## Presets")
    add("")
    add(
        "Renderers in common use have a ready-made preset; the rest are selected with "
        "`-DCNA_GRAPHICS_RENDERER=<NAME>` against any preset or a plain build directory. "
        "Every one of the "
        f"{count} renderers above is selectable either way."
    )
    add("")
    add("```bash")
    add("cmake --list-presets                 # what is available")
    add("cmake --preset opengles3             # configure")
    add("cmake --build --preset opengles3     # build")
    add("ctest --preset opengles3             # smoke test")
    add("```")
    add("")

    if canonical:
        add(
            f"<sub>Cross-checked against CNA's own canonical list of {len(canonical)} "
            "renderers at generation time.</sub>"
        )
        add("")

    return "\n".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify the generated files are up to date instead of writing them",
    )
    parser.add_argument(
        "--ci-matrix",
        metavar="PLATFORM",
        help="print a GitHub Actions matrix of renderers for PLATFORM and exit",
    )
    parser.add_argument(
        "--tiers",
        default="B",
        help="comma-separated CI tiers to include with --ci-matrix (default: B)",
    )
    args = parser.parse_args()

    renderers = load_manifest()

    # Lets the CI workflow build its matrix from the manifest instead of
    # repeating a renderer list in YAML, where it would drift unnoticed.
    if args.ci_matrix:
        tiers = {t.strip() for t in args.tiers.split(",") if t.strip()}
        selected = [
            r["name"]
            for r in renderers
            if args.ci_matrix in r["platforms"] and r["tier"] in tiers
        ]
        print(json.dumps(selected))
        return 0

    for line in check_against_cna(renderers):
        print(line)

    wanted = {
        PRESETS: json.dumps(build_presets(renderers), indent=2) + "\n",
        RENDERER_DOC: build_doc(renderers),
    }

    if args.check:
        stale = []
        for path, content in wanted.items():
            current = path.read_text() if path.exists() else None
            if current != content:
                stale.append(path.relative_to(REPO))
        if stale:
            print()
            print("Stale generated file(s): " + ", ".join(str(p) for p in stale))
            print("Run: python3 tools/gen_renderer_files.py")
            return 1
        print(f"ok: {len(wanted)} generated file(s) up to date")
        return 0

    for path, content in wanted.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
        print(f"wrote {path.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
