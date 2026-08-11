# =============================================================================
# cna-template — a starter project for the CNA game framework.
# =============================================================================
# Everything renderer-specific in this file is driven by cmake/renderers.json
# (see cmake/CnaRenderers.cmake). There is deliberately no per-renderer
# if/elseif chain here: CNA has 46 renderers and keeps growing, so the template
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
# CNA also exposes 46 CNA_RENDERER_<NAME> booleans as an alternative input path.
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

# CNA has now resolved the renderer (ours or its own default) and published the
# canonical list in the cache. Re-check both against the manifest.
cna_template_assert_no_drift()
cna_template_validate_renderer("${CNA_GRAPHICS_RENDERER}" "${CNA_ROOT_DIR}")

cna_template_renderer_field("${CNA_GRAPHICS_RENDERER}" display   CNA_TEMPLATE_RENDERER_DISPLAY)
cna_template_renderer_field("${CNA_GRAPHICS_RENDERER}" dimension CNA_TEMPLATE_RENDERER_DIMENSION)
message(STATUS "cna-template: renderer ${CNA_GRAPHICS_RENDERER} "
               "(${CNA_TEMPLATE_RENDERER_DIMENSION}, display: ${CNA_TEMPLATE_RENDERER_DISPLAY})")

# SDL3's IMPORTED targets are created by find_package() inside CNA's directory
# scope, so they are not visible up here. Re-importing is cheap and idempotent:
# CNA has already cached SDL3_DIR pointing at the SDL it built. This must come
# after add_subdirectory() for exactly that reason.
if(NOT TARGET SDL3::SDL3)
    find_package(SDL3 REQUIRED CONFIG)
endif()

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
# selected renderer target, the public compile definitions and the public
# include directories. Linking a renderer target by hand is obsolete, and so is
# the old -Wl,--start-group wrapper: CNA declares its static-library cycles on
# the targets themselves, and a linker group around an INTERFACE library
# expands to nothing anyway.
target_link_libraries(${_app} PRIVATE CNA SHARP_RUNTIME)

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
    # Run outside MSYS2 without shipping libgcc_s_seh-1.dll / libstdc++-6.dll.
    target_link_options(${_app} PRIVATE -static-libgcc -static-libstdc++)
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
