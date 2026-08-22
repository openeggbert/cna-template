# =============================================================================
# cna-template — a starter project for the CNA game framework.
# =============================================================================
# Everything renderer-specific in this file is driven by cmake/renderers.json
# (see cmake/CnaRenderers.cmake). There is deliberately no per-renderer
# if/elseif chain here: CNA has 50 renderers and keeps growing, so the template
# treats the renderer set as data rather than as code.
#
# CMake 3.23 is the floor because the easy-gl/meta-gl siblings backing the
# OPENGLES2/OPENGLES3/OPENGL33/WEBGL1/WEBGL2 renderers require it (FILE_SET
# HEADERS). CNA itself only needs 3.20, but a template that could not build the
# GL family would be a poor starting point.
cmake_minimum_required(VERSION 3.23)

project(CnaTemplate VERSION 0.1.0 LANGUAGES C CXX)

# --- Customization point: rename your application here -----------------------
# Drives the executable name and the CTest name. Renaming the C++ class itself
# is a separate step -- see "Making this your own project" in README.md.
set(CNA_TEMPLATE_APP_NAME "HelloGame" CACHE STRING
    "Name of the application executable produced by this template")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CnaRenderers)

message(STATUS "cna-template: ${PROJECT_NAME} -> ${CNA_TEMPLATE_APP_NAME}")
message(STATUS "cna-template: platform ${CMAKE_SYSTEM_NAME}, "
               "compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# -----------------------------------------------------------------------------
# Upstream compatibility preflight
# -----------------------------------------------------------------------------
# CNA's module-layout validator (cna/modules/CMakeLists.txt) aborts the build if
# a directory named src/ or include/ exists at ${CMAKE_SOURCE_DIR}. When CNA is
# consumed with add_subdirectory(), CMAKE_SOURCE_DIR is *this* project rather
# than CNA -- so any consumer using the conventional src/ + include/ layout dies
# with the confusing message "legacy global 'src/' tree reappeared at the
# repository root". That is an upstream bug; see missing.md.
#
# This template therefore keeps its sources under game/. The check below exists
# so that if you add src/ or include/ back you are told why it breaks, instead
# of being sent to go looking inside CNA.
foreach(_cna_template_reserved IN ITEMS src include)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${_cna_template_reserved}")
        message(FATAL_ERROR
            "cna-template: a directory named '${_cna_template_reserved}/' at the top level of\n"
            "this project makes CNA's module-layout validator abort the configure.\n"
            "\n"
            "This is an upstream CNA bug: cna/modules/CMakeLists.txt tests\n"
            "  \${CMAKE_SOURCE_DIR}/${_cna_template_reserved}\n"
            "which is the CONSUMER's root when CNA is added with add_subdirectory(),\n"
            "not CNA's own root. Until it is fixed upstream, keep application sources\n"
            "elsewhere -- this template uses game/src and game/include.\n"
            "\n"
            "missing.md has the one-line upstream fix.")
    endif()
endforeach()

# -----------------------------------------------------------------------------
# CNA framework location
# -----------------------------------------------------------------------------
# CNA is a sibling checkout, not a submodule, and is consumed with
# add_subdirectory(): CNA installs and exports no CMake package, so
# find_package(CNA) does not exist.
set(CNA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cna" CACHE PATH
    "Path to the CNA framework checkout")

if(NOT EXISTS "${CNA_ROOT_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "cna-template: CNA framework not found at '${CNA_ROOT_DIR}'.\n"
        "CNA is a separate sibling repository, not part of this template:\n"
        "  cd ${CMAKE_CURRENT_SOURCE_DIR}/.. && git clone https://github.com/openeggbert/cna.git\n"
        "or pass -DCNA_ROOT_DIR=/path/to/cna to cmake.\n"
        "README.md \"Prerequisites\" lists every checkout you need.")
endif()

# -----------------------------------------------------------------------------
# Renderer selection
# -----------------------------------------------------------------------------
# CNA_GRAPHICS_RENDERER is CNA's own public cache variable and this template
# passes it straight through -- it deliberately does not re-declare a default.
# If you set nothing, CNA picks its own (WEBGL2 on the web, OPENGLES3 on Linux,
# SDL_RENDERER elsewhere) and the template validates the result afterwards.
#
# CNA also exposes 50 CNA_RENDERER_<NAME> booleans as an alternative input path.
# They all default to OFF and CNA only consults them if one is switched ON, so
# setting the string alone is sufficient and this file leaves them alone.
if(DEFINED CNA_GRAPHICS_RENDERER AND NOT CNA_GRAPHICS_RENDERER STREQUAL "")
    cna_template_validate_renderer("${CNA_GRAPHICS_RENDERER}" "${CNA_ROOT_DIR}")
    message(STATUS "cna-template: renderer ${CNA_GRAPHICS_RENDERER} (requested)")
else()
    message(STATUS "cna-template: no renderer requested; using CNA's default for this platform")
endif()

# -----------------------------------------------------------------------------
# CNA build options
# -----------------------------------------------------------------------------
# Keep CNA's own footprint small: an application needs neither the framework's
# test suite nor its demo applications. easy-gl's gates default ON and it is
# pulled in for the whole GL renderer family, so silence those too. meta-gl's
# own tests already default OFF, so no BUILD_TESTING override is needed.
set(CNA_BUILD_TESTS       OFF CACHE BOOL "Build CNA tests"                FORCE)
set(CNA_BUILD_EXAMPLES    OFF CACHE BOOL "Build CNA example applications" FORCE)
set(EASYGL_BUILD_TESTS    OFF CACHE BOOL "Build easy-gl tests"            FORCE)
set(EASYGL_BUILD_EXAMPLES OFF CACHE BOOL "Build easy-gl examples"         FORCE)

# CNA's vendored SDL cache defaults inside CNA's own source checkout. That is
# unsuitable for immutable dependency checkouts and failed during the web
# verification here. Keep the persistent, target-keyed cache in this
# consumer's ignored build/ area unless the caller chose another location.
# See missing.md CNA-9.
if(NOT DEFINED CNA_SDL_PREBUILT_ROOT)
    if(EMSCRIPTEN)
        set(_cna_sdl_cache_target emscripten)
    else()
        set(_cna_sdl_cache_target
            "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_C_COMPILER_ID}")
    endif()
    set(CNA_SDL_PREBUILT_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/build/cna-sdl-prebuilt-${_cna_sdl_cache_target}"
        CACHE PATH "Persistent CNA SDL build cache outside the CNA checkout")
endif()

# CNA's modular consumption layer owns the exact Sharp Runtime component
# closure its public targets need. CNA currently loads that layer only after it
# has already added sharp-runtime, so an embedded build otherwise falls back to
# SharpRuntime::All and needlessly requires optional dependencies such as zlib
# on every cross-target. Read CNA's own list before add_subdirectory() rather
# than duplicating it here. An application that directly uses extra Sharp
# Runtime APIs can set SHARP_RUNTIME_COMPONENTS itself and link their narrow
# SharpRuntime::<Component> targets.
set(_cna_sharp_runtime_consumption
    "${CNA_ROOT_DIR}/cmake/SharpRuntimeConsumption.cmake")
if(EXISTS "${_cna_sharp_runtime_consumption}")
    include("${_cna_sharp_runtime_consumption}")
    if(NOT DEFINED SHARP_RUNTIME_COMPONENTS)
        set(SHARP_RUNTIME_COMPONENTS "${CNA_SHARP_RUNTIME_DEFAULT_COMPONENTS}"
            CACHE STRING "Sharp Runtime components required by CNA")
    endif()
endif()

# Same upstream root cause as the src/include guard above: CNA's content module
# adds its vendored single-header include paths as ${CMAKE_SOURCE_DIR}/third_party/...,
# which resolves into the CONSUMER's tree rather than CNA's. Building CNA as a
# subdirectory therefore fails with "fatal error: cgltf.h: No such file or
# directory". INCLUDE_DIRECTORIES is a directory property that subdirectories
# inherit, so setting the correct paths here repairs the search path without
# modifying CNA. Recorded in missing.md; remove once CNA uses its own root.
foreach(_cna_vendored_header_dir IN ITEMS cgltf stb)
    if(EXISTS "${CNA_ROOT_DIR}/third_party/${_cna_vendored_header_dir}")
        include_directories("${CNA_ROOT_DIR}/third_party/${_cna_vendored_header_dir}")
    endif()
endforeach()

add_subdirectory("${CNA_ROOT_DIR}" CNA)

# CNA's modular targets correctly use SharpRuntime::<Component>, but a few
# ancillary upstream tools still link the legacy SHARP_RUNTIME name directly.
# The modular runtime intentionally creates that compatibility target only for
# the expensive All selection.  Supply an equivalent bridge over CNA's narrow
# component set so those tools remain buildable without enabling every optional
# component (and its cross-platform dependencies).  See missing.md CNA-7.
if(NOT TARGET SHARP_RUNTIME
        AND TARGET SharpRuntime::Core.Base
        AND DEFINED CNA_SHARP_RUNTIME_DEFAULT_COMPONENTS)
    add_library(SHARP_RUNTIME INTERFACE)
    foreach(_sharp_component IN LISTS CNA_SHARP_RUNTIME_DEFAULT_COMPONENTS)
        target_link_libraries(SHARP_RUNTIME INTERFACE
            "SharpRuntime::${_sharp_component}")
    endforeach()
endif()

# Older CNA revisions changed IGraphicsRenderer::CreateRenderTargetCube() from
# four to five parameters but missed CanvasRenderer's declaration and
# definition. Keep the sibling checkout read-only: when that exact stale pair
# is present, derive two corrected build-tree compatibility copies. Current CNA
# already contains the fix, in which case no overlay is needed. See missing.md
# CNA-8.
if(EMSCRIPTEN
        AND CNA_GRAPHICS_RENDERER STREQUAL "CANVAS"
        AND TARGET cna_renderer_canvas)
    set(_canvas_source
        "${CNA_ROOT_DIR}/modules/renderers/canvas/src/CanvasRenderer.cpp")
    set(_canvas_header
        "${CNA_ROOT_DIR}/modules/renderers/canvas/include/CNA/Internal/Renderers/Canvas/CanvasRenderer.hpp")
    cmake_path(NORMAL_PATH _canvas_source)
    cmake_path(NORMAL_PATH _canvas_header)
    set(_canvas_compat_root
        "${CMAKE_CURRENT_BINARY_DIR}/cna-template-compat/canvas")
    set(_canvas_compat_source "${_canvas_compat_root}/CanvasRenderer.cpp")
    set(_canvas_compat_header
        "${_canvas_compat_root}/include/CNA/Internal/Renderers/Canvas/CanvasRenderer.hpp")

    file(READ "${_canvas_header}" _canvas_header_content)
    set(_canvas_old_header_signature [=[
        std::unique_ptr<IRenderTargetCubeRenderer> CreateRenderTargetCube(
            int size, int depthFormat, bool mipMap = false,
            int multiSampleCount = 0) override;]=])
    set(_canvas_new_header_signature [=[
        std::unique_ptr<IRenderTargetCubeRenderer> CreateRenderTargetCube(
            int size, int depthFormat, bool preserveContents = false,
            bool mipMap = false, int multiSampleCount = 0) override;]=])
    string(FIND "${_canvas_header_content}"
        "${_canvas_old_header_signature}" _canvas_header_signature_position)
    string(FIND "${_canvas_header_content}"
        "int size, int depthFormat, bool preserveContents = false"
        _canvas_new_header_signature_position)

    file(READ "${_canvas_source}" _canvas_source_content)
    set(_canvas_old_source_signature [=[
    std::unique_ptr<IRenderTargetCubeRenderer> CanvasRenderer::CreateRenderTargetCube(
        int size, int, bool, int)]=])
    set(_canvas_new_source_signature [=[
    std::unique_ptr<IRenderTargetCubeRenderer> CanvasRenderer::CreateRenderTargetCube(
        int size, int, bool, bool, int)]=])
    string(FIND "${_canvas_source_content}"
        "${_canvas_old_source_signature}" _canvas_source_signature_position)
    string(FIND "${_canvas_source_content}"
        "${_canvas_new_source_signature}" _canvas_new_source_signature_position)

    if(NOT _canvas_header_signature_position EQUAL -1
            AND NOT _canvas_source_signature_position EQUAL -1)
        string(REPLACE "${_canvas_old_header_signature}"
            "${_canvas_new_header_signature}"
            _canvas_header_content "${_canvas_header_content}")
        string(REPLACE "${_canvas_old_source_signature}"
            "${_canvas_new_source_signature}"
            _canvas_source_content "${_canvas_source_content}")

        file(MAKE_DIRECTORY
            "${_canvas_compat_root}"
            "${_canvas_compat_root}/include/CNA/Internal/Renderers/Canvas")
        file(CONFIGURE OUTPUT "${_canvas_compat_header}"
            CONTENT "${_canvas_header_content}" @ONLY)
        file(CONFIGURE OUTPUT "${_canvas_compat_source}"
            CONTENT "${_canvas_source_content}" @ONLY)

        get_target_property(_canvas_sources cna_renderer_canvas SOURCES)
        list(FIND _canvas_sources "${_canvas_source}" _canvas_source_index)
        if(_canvas_source_index EQUAL -1)
            message(FATAL_ERROR
                "cna-template: cannot locate CanvasRenderer.cpp in "
                "cna_renderer_canvas; refusing an incomplete CNA-8 workaround.")
        endif()
        list(REMOVE_ITEM _canvas_sources "${_canvas_source}")
        set_property(TARGET cna_renderer_canvas PROPERTY SOURCES
            ${_canvas_sources} "${_canvas_compat_source}")
        target_include_directories(cna_renderer_canvas BEFORE PRIVATE
            "${_canvas_compat_root}/include")
    elseif(NOT _canvas_new_header_signature_position EQUAL -1
            AND NOT _canvas_new_source_signature_position EQUAL -1)
        message(STATUS
            "cna-template: CNA already contains the Canvas render-target fix")
    else()
        message(FATAL_ERROR
            "cna-template: CNA's Canvas CreateRenderTargetCube signatures do "
            "not match either the known stale pair or the corrected pair. "
            "Update the CNA-8 compatibility block instead of applying it blindly.")
    endif()
endif()

# CNA has now resolved the renderer (ours or its own default) and published the
# canonical list in the cache. Re-check both against the manifest.
cna_template_assert_no_drift()
cna_template_validate_renderer("${CNA_GRAPHICS_RENDERER}" "${CNA_ROOT_DIR}")

cna_template_renderer_field("${CNA_GRAPHICS_RENDERER}" display   CNA_TEMPLATE_RENDERER_DISPLAY)
cna_template_renderer_field("${CNA_GRAPHICS_RENDERER}" dimension CNA_TEMPLATE_RENDERER_DIMENSION)
message(STATUS "cna-template: renderer ${CNA_GRAPHICS_RENDERER} "
               "(${CNA_TEMPLATE_RENDERER_DIMENSION}, display: ${CNA_TEMPLATE_RENDERER_DISPLAY})")

# sharp-runtime's RandomAccess.cpp defines the native-only ThrowNative helper
# on Emscripten too. The platform branches never call it there, so the module's
# own -Werror turns Clang's -Wunused-function warning into a build failure.
# Keep the compatibility exception on that one source file; see missing.md
# SHARP-RUNTIME-2. Remove this once upstream guards or annotates the helper.
if(EMSCRIPTEN AND TARGET sharp_runtime_io)
    set_property(SOURCE
        "${CNA_SHARP_RUNTIME_ROOT}/modules/io/src/System/IO/RandomAccess.cpp"
        TARGET_DIRECTORY sharp_runtime_io
        APPEND PROPERTY COMPILE_OPTIONS -Wno-error=unused-function)
endif()

# SDL's IMPORTED targets are created by find_package() inside CNA's directory
# scope, so they are not visible up here. Re-importing is cheap and idempotent:
# CNA has already cached each *_DIR pointing at the libraries it built. All
# three must be visible here so cna_copy_sdl_runtime() can package their DLLs.
# This must come after add_subdirectory() for exactly that reason.
foreach(_sdl_package IN ITEMS SDL3 SDL3_image SDL3_mixer)
    if(NOT TARGET ${_sdl_package}::${_sdl_package})
        find_package(${_sdl_package} REQUIRED CONFIG)
    endif()
endforeach()

# -----------------------------------------------------------------------------
# The application
# -----------------------------------------------------------------------------
set(CNA_TEMPLATE_SOURCES
    game/src/HelloGame.cpp
    game/src/Program.cpp
)

# On Android the "executable" is a shared library that SDL's Java glue loads by
# name, so the target name is fixed regardless of CNA_TEMPLATE_APP_NAME.
if(ANDROID)
    set(_app main)
    add_library(${_app} SHARED ${CNA_TEMPLATE_SOURCES})
else()
    set(_app ${CNA_TEMPLATE_APP_NAME})
    add_executable(${_app} ${CNA_TEMPLATE_SOURCES})
endif()

target_include_directories(${_app} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/game/include")

# CNA propagates CNA_RENDERER_<X> to consumers through its cna_build_flags
# INTERFACE, but applies CNA_GL_PROFILE_<X> with a directory-scoped
# add_compile_definitions() that never reaches a parent-scope consumer. The five
# GL renderers all share the define CNA_RENDERER_EASYGL, so without the profile
# CNA::getCurrentGraphicsRendererName() falls through to its OPENGLES3 default
# and misreports OPENGLES2/OPENGL33/WEBGL1/WEBGL2 in *our* translation units
# while CNA's own report the truth. Re-applying it here restores agreement.
# Upstream fix recorded in missing.md.
cna_template_renderer_field("${CNA_GRAPHICS_RENDERER}" category _renderer_category)
if(_renderer_category STREQUAL "gl-easygl")
    target_compile_definitions(${_app} PRIVATE "CNA_GL_PROFILE_${CNA_GRAPHICS_RENDERER}")
endif()

# CNA is an INTERFACE umbrella already carrying every framework module, the
# selected renderer target, the required Sharp Runtime component targets, the
# public compile definitions and the public include directories. Linking either
# a renderer or the legacy all-components SHARP_RUNTIME umbrella by hand is
# obsolete. So is the old -Wl,--start-group wrapper: CNA declares its
# static-library cycles on the targets themselves, and a linker group around an
# INTERFACE library expands to nothing anyway.
target_link_libraries(${_app} PRIVATE CNA)

if(TARGET SDL3::SDL3main)
    target_link_libraries(${_app} PRIVATE SDL3::SDL3main)
endif()

# -----------------------------------------------------------------------------
# Platform specifics
# -----------------------------------------------------------------------------
if(ANDROID)
    # CNA links android/log PUBLIC on its own device module, so they arrive
    # through the umbrella; only SDL3 has to be named here.
    target_link_libraries(${_app} PRIVATE SDL3::SDL3)
    # Assets are packaged into the APK -- see android/app/build.gradle.

elseif(EMSCRIPTEN)
    target_link_libraries(${_app} PRIVATE SDL3::SDL3)
    set_target_properties(${_app} PROPERTIES SUFFIX ".html")

    # CNA enables the Wasm exception-handling ABI in its own directory scope.
    # Directory properties propagate downward only, so this target -- defined in
    # the parent scope -- must opt in explicitly or the link fails with
    # "wasm-ld: undefined symbol: __cpp_exception".
    target_compile_options(${_app} PRIVATE -fwasm-exceptions)
    target_link_options(${_app} PRIVATE -fwasm-exceptions)

    target_link_options(${_app} PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1048576
        -sINITIAL_MEMORY=134217728
        -sFORCE_FILESYSTEM=1
        # Let main() actually return, so --smoke-test terminates the process.
        -sEXIT_RUNTIME=1
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Content@/Content")

    # WebGL version constraints are per renderer, not global: pinning
    # MIN/MAX_WEBGL_VERSION=2 for every web build would break WEBGL1, and the
    # CANVAS/HTML_DOM/SVG_DOM renderers create no WebGL context at all.
    cna_template_renderer_field("${CNA_GRAPHICS_RENDERER}" webflags _webflags)
    if(_webflags)
        target_link_options(${_app} PRIVATE ${_webflags})
    endif()

else()
    # Native desktop: put Content/ next to the executable so the relative paths
    # the game uses at runtime resolve.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Content")
        add_custom_command(TARGET ${_app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/Content"
                "$<TARGET_FILE_DIR:${_app}>/Content"
            COMMENT "Copying game content next to ${_app}")
    endif()
endif()

if(WIN32)
    # CNA defines these helpers; CMake functions are global once defined, so they
    # are callable here after add_subdirectory(). Fall back to CMake's own
    # TARGET_RUNTIME_DLLS if a CNA revision does not provide them.
    if(COMMAND cna_copy_sdl_runtime)
        cna_copy_sdl_runtime(${_app})
    else()
        add_custom_command(TARGET ${_app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_RUNTIME_DLLS:${_app}>" "$<TARGET_FILE_DIR:${_app}>"
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to ${_app}")
    endif()
endif()

if(MINGW)
    # CNA's full RTTI graph is not compatible with MinGW's static libstdc++:
    # PE-COFF fails to fold the inline std::type_info::operator== emitted by
    # cna_runtime against the identical copy in libstdc++.a. Use the dynamic
    # runtime and ask CNA to package its DLLs next to the executable instead.
    if(COMMAND cna_copy_mingw_cxx_runtime)
        cna_copy_mingw_cxx_runtime(${_app})
    else()
        message(WARNING
            "cna-template: this CNA revision does not provide "
            "cna_copy_mingw_cxx_runtime(); copy the MinGW GCC and C++ runtime "
            "DLLs next to ${_app} before running it.")
    endif()
endif()

# -----------------------------------------------------------------------------
# Tests
# -----------------------------------------------------------------------------
option(CNA_TEMPLATE_BUILD_TESTS "Register cna-template's own smoke test" ON)

if(CNA_TEMPLATE_BUILD_TESTS AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_test(NAME ${CNA_TEMPLATE_APP_NAME}Smoke COMMAND ${_app} --smoke-test)

    # Label the test by what it needs from the environment, so CI can run the
    # display-free renderers with no X server at all and reserve xvfb-run for
    # the ones that genuinely open a window:
    #   ctest -L headless               # no display required
    #   xvfb-run -a ctest -L display
    if(CNA_TEMPLATE_RENDERER_DISPLAY STREQUAL "none")
        set(_labels headless)
    else()
        set(_labels display)
    endif()

    set_tests_properties(${CNA_TEMPLATE_APP_NAME}Smoke PROPERTIES
        TIMEOUT 30
        LABELS "${_labels}")
endif()
