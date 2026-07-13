cmake_minimum_required(VERSION 3.21)
project(CnaTemplate LANGUAGES C CXX)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# CNA framework location
# -----------------------------------------------------------------------------
# CNA is a SIBLING repository checkout, not a git submodule of this template
# (same convention as mobile-eggbert / cna-samples). Override with
# -DCNA_ROOT_DIR=/path/to/cna if your checkout uses a different directory
# name or location.
set(CNA_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cna" CACHE PATH
    "Path to the CNA framework checkout")

if(NOT EXISTS "${CNA_ROOT_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "cna-template: CNA framework not found at '${CNA_ROOT_DIR}'.\n"
        "CNA is a separate sibling repository, not part of this template. "
        "Clone it next to this project, e.g.:\n"
        "  cd ${CMAKE_CURRENT_SOURCE_DIR}/.. && git clone https://github.com/openeggbert/cna.git\n"
        "or pass -DCNA_ROOT_DIR=/path/to/cna to cmake.")
endif()

# -----------------------------------------------------------------------------
# Graphics backend selection for CNA.
# CNA_GRAPHICS_BACKEND is the single public selection interface. The boolean
# CNA_BACKEND_* cache entries below are implementation details required by CNA.
# Available backends: SDL_RENDERER, EASYGL, BGFX, VULKAN, WEBGPU
# -----------------------------------------------------------------------------
if(EMSCRIPTEN OR ANDROID)
    # On Emscripten/Android only SDL_RENDERER is supported (no desktop GL context
    # the same way; SDL_RENDERER runs on WebGL/GLES under the hood).
    set(CNA_BACKEND_SDL_RENDERER ON  CACHE BOOL "Enable SDL_Renderer backend in CNA" FORCE)
    set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "Enable easy-gl backend in CNA"      FORCE)
    set(CNA_BACKEND_BGFX         OFF CACHE BOOL "Enable bgfx backend in CNA"         FORCE)
    set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "Enable Vulkan backend in CNA"       FORCE)
    set(CNA_BACKEND_WEBGPU       OFF CACHE BOOL "Enable WebGPU backend in CNA"       FORCE)
    set(CNA_GRAPHICS_BACKEND "SDL_RENDERER" CACHE STRING "Selected CNA backend (SDL_RENDERER, EASYGL, BGFX, VULKAN, WEBGPU)" FORCE)
else()
    # Desktop: choose one backend. WEBGPU is experimental and requires a CNA
    # checkout that exposes cna_backend_graphics_webgpu.
    set(CNA_GRAPHICS_BACKEND "SDL_RENDERER" CACHE STRING "Selected CNA backend (SDL_RENDERER, EASYGL, BGFX, VULKAN, WEBGPU)")
    set_property(CACHE CNA_GRAPHICS_BACKEND PROPERTY STRINGS SDL_RENDERER EASYGL BGFX VULKAN WEBGPU)

    set(_cna_template_backends SDL_RENDERER EASYGL BGFX VULKAN WEBGPU)
    list(FIND _cna_template_backends "${CNA_GRAPHICS_BACKEND}" _cna_template_backend_index)
    if(_cna_template_backend_index EQUAL -1)
        message(FATAL_ERROR
            "cna-template: unknown CNA_GRAPHICS_BACKEND='${CNA_GRAPHICS_BACKEND}'. "
            "Choose one of: ${_cna_template_backends}.")
    endif()

    if(CNA_GRAPHICS_BACKEND STREQUAL "SDL_RENDERER")
        set(CNA_BACKEND_SDL_RENDERER ON  CACHE BOOL "" FORCE)
        set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_BGFX         OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_WEBGPU       OFF CACHE BOOL "" FORCE)
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "EASYGL")
        set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_EASY_GL      ON  CACHE BOOL "" FORCE)
        set(CNA_BACKEND_BGFX         OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_WEBGPU       OFF CACHE BOOL "" FORCE)
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "BGFX")
        set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_BGFX         ON  CACHE BOOL "" FORCE)
        set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_WEBGPU       OFF CACHE BOOL "" FORCE)
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "VULKAN")
        set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_BGFX         OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_VULKAN       ON  CACHE BOOL "" FORCE)
        set(CNA_BACKEND_WEBGPU       OFF CACHE BOOL "" FORCE)
    elseif(CNA_GRAPHICS_BACKEND STREQUAL "WEBGPU")
        set(CNA_BACKEND_SDL_RENDERER OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_EASY_GL      OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_BGFX         OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_VULKAN       OFF CACHE BOOL "" FORCE)
        set(CNA_BACKEND_WEBGPU       ON  CACHE BOOL "" FORCE)
    endif()

endif()

# Keep CNA's own configure/build footprint small: this template doesn't need
# CNA's own test suite or its example applications built alongside HelloGame.
# Also silence the EASYGL backend's own vendored dependencies (easy-gl, and
# its own dependency meta-gl) so their internal test suites don't get swept
# into cna-template's own `ctest` run -- found by actually running `ctest`
# and seeing 5 unrelated, never-built easy-gl/meta-gl test executables show
# up as failures ("Not Run") alongside our own HelloGameSmoke. Testing those
# libraries is their own CI's job, not this template's. meta-gl's own gate is
# literally named BUILD_TESTING (not a template-specific name), and CMake's
# option() never overwrites an existing cache entry, so this must run before
# add_subdirectory(CNA) pulls it in transitively, and cna-template's own test
# below deliberately does NOT use BUILD_TESTING for exactly this reason (see
# the CNA_TEMPLATE_BUILD_TESTS option further down).
set(CNA_BUILD_TESTS    OFF CACHE BOOL "Build CNA tests"                FORCE)
set(CNA_BUILD_EXAMPLES OFF CACHE BOOL "Build CNA example applications" FORCE)
set(EASYGL_BUILD_TESTS    OFF CACHE BOOL "Build easy-gl tests"          FORCE)
set(EASYGL_BUILD_EXAMPLES OFF CACHE BOOL "Build easy-gl examples"       FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build the test suite" FORCE)

add_subdirectory("${CNA_ROOT_DIR}" CNA)

set(HELLOGAME_SOURCES
    src/HelloGame/HelloGame.cpp
    src/HelloGame/Program.cpp
)

# On Android the "executable" is a shared library loaded by SDLActivity.
# SDL's Java glue expects the native library to be named "libmain.so".
if(ANDROID)
    set(_game_target main)
    add_library(${_game_target} SHARED ${HELLOGAME_SOURCES})
else()
    set(_game_target HelloGame)
    add_executable(${_game_target} ${HELLOGAME_SOURCES})
endif()

# On Emscripten the output is HTML+JS+WASM; set the suffix accordingly.
if(EMSCRIPTEN)
    set_target_properties(${_game_target} PROPERTIES SUFFIX ".html")
endif()

target_include_directories(${_game_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# CNA builds SDL3 once into its own prebuilt root and sets SDL3_DIR / SDL3_image_DIR /
# SDL3_mixer_DIR in the CMake cache. SDL IMPORTED targets are directory-scoped, so we
# must call find_package() here to make SDL3::SDL3 visible in this project's scope.
find_package(SDL3       REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)
find_package(SDL3_mixer REQUIRED CONFIG)

# CNA and the selected graphics backend static library have circular symbol
# references on Linux (backend uses Color/Rectangle/Matrix from CNA, CNA uses
# backend interfaces). Wrap them in a linker group so GNU ld resolves both
# without depending on link order.
# Note: --start-group/--end-group are not available on Emscripten's linker or
# on Android (lld does not need it and warns).
set(_cna_backend_target "")
if(CNA_GRAPHICS_BACKEND STREQUAL "SDL_RENDERER")
    set(_cna_backend_target cna_backend_graphics_sdl_renderer)
elseif(CNA_GRAPHICS_BACKEND STREQUAL "EASYGL")
    set(_cna_backend_target cna_backend_graphics_easygl)
elseif(CNA_GRAPHICS_BACKEND STREQUAL "BGFX")
    set(_cna_backend_target cna_backend_graphics_bgfx)
elseif(CNA_GRAPHICS_BACKEND STREQUAL "VULKAN")
    set(_cna_backend_target cna_backend_graphics_vulkan)
elseif(CNA_GRAPHICS_BACKEND STREQUAL "WEBGPU")
    set(_cna_backend_target cna_backend_graphics_webgpu)
endif()

if(NOT TARGET ${_cna_backend_target})
    message(FATAL_ERROR
        "cna-template: CNA_GRAPHICS_BACKEND=${CNA_GRAPHICS_BACKEND} was requested, "
        "but this CNA checkout does not define target '${_cna_backend_target}'. "
        "Use a CNA revision that supports this backend or select another backend.")
endif()

if(EMSCRIPTEN)
    target_link_libraries(${_game_target} PRIVATE CNA SDL3::SDL3-static)
elseif(ANDROID)
    target_link_libraries(${_game_target} PRIVATE CNA SDL3::SDL3 android log)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32)
    target_link_libraries(${_game_target} PRIVATE
        -Wl,--start-group CNA ${_cna_backend_target} -Wl,--end-group
        SDL3::SDL3)
else()
    target_link_libraries(${_game_target} PRIVATE CNA SDL3::SDL3)
endif()

if(TARGET SDL3::SDL3main)
    target_link_libraries(${_game_target} PRIVATE SDL3::SDL3main)
endif()

if(ANDROID)
    # No content copy needed on Android: assets are packaged into the APK
    # (see android/app/build.gradle sourceSets.main.assets.srcDirs).
elseif(EMSCRIPTEN)
    # Preload Content/ into the Emscripten virtual filesystem at /Content,
    # matching the relative path the game uses at runtime.
    target_link_options(${_game_target} PRIVATE
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=1048576
        -sINITIAL_MEMORY=134217728
        -sFORCE_FILESYSTEM=1
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Content@/Content"
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
    )
else()
    # Native desktop: copy Content/ next to the executable.
    set(CNA_TEMPLATE_CONTENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Content")
    if(EXISTS "${CNA_TEMPLATE_CONTENT_DIR}")
        add_custom_command(TARGET ${_game_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CNA_TEMPLATE_CONTENT_DIR}"
                "$<TARGET_FILE_DIR:${_game_target}>/Content"
                COMMENT "Copying game content next to executable")
    endif()
endif()

if(WIN32)
    # Copy SDL3/SDL3_image/SDL3_mixer (and any other imported runtime DLL
    # dependency, including MinGW's libwinpthread) next to the executable.
    # Uses CMake's own TARGET_RUNTIME_DLLS generator expression (CMake >= 3.21)
    # instead of a CNA-internal helper function, since functions defined via
    # include() while CNA is processed as our subdirectory are not visible
    # back in this top-level scope.
    add_custom_command(TARGET ${_game_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_RUNTIME_DLLS:${_game_target}>" "$<TARGET_FILE_DIR:${_game_target}>"
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to executable")
endif()

if(MINGW)
    # Statically link the MinGW GCC and C++ runtime so the exe runs outside
    # MSYS2 without needing libgcc_s_seh-1.dll / libstdc++-6.dll.
    target_link_options(${_game_target} PRIVATE -static-libgcc -static-libstdc++)
endif()

# A dedicated option rather than CTest's own BUILD_TESTING: BUILD_TESTING is
# forced OFF above specifically so easy-gl/meta-gl's own test suites don't
# leak into this project's ctest run, so reusing that same name here would
# just disable our own test as a side effect of disabling theirs.
option(CNA_TEMPLATE_BUILD_TESTS "Register cna-template's own HelloGameSmoke CTest" ON)
if(CNA_TEMPLATE_BUILD_TESTS AND NOT ANDROID AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_test(NAME HelloGameSmoke COMMAND ${_game_target} --smoke-test)
    # No SDL_VIDEODRIVER override here: "dummy" only works for SDL_RENDERER
    # (confirmed by actually running it -- EASYGL/BGFX/VULKAN need a real or
    # virtual GL-capable display and throw "OpenGL support is ... not
    # available in current SDL video driver (dummy)" otherwise). Run this
    # test under SDL_VIDEODRIVER=dummy directly for SDL_RENDERER, or under
    # `xvfb-run` for any GL-capable backend -- see README's Tests section.
    set_tests_properties(HelloGameSmoke PROPERTIES TIMEOUT 15)
endif()
