# =============================================================================
# Renderer knowledge for cna-template.
# =============================================================================
# CNA owns the list of renderer names. This template owns only the extra
# metadata CNA does not publish (platform applicability, display requirement,
# CI tier, ...), which lives in cmake/renderers.json.
#
# The two are reconciled at configure time, twice:
#
#   1. cna_template_canonical_renderers() parses the authoritative
#      set_property(CACHE CNA_GRAPHICS_RENDERER PROPERTY STRINGS ...) line out
#      of the CNA checkout BEFORE add_subdirectory(), so a bad renderer name is
#      rejected with a useful message instead of dying inside CNA.
#
#   2. cna_template_assert_no_drift() re-reads the same list from the CMake
#      cache AFTER add_subdirectory(), which is authoritative rather than
#      textual, and fails if the manifest and CNA have diverged.
#
# So a renderer added to CNA cannot silently go missing here, and a renderer
# removed from CNA cannot silently linger.
# =============================================================================

include_guard(GLOBAL)

# CMake renders a list as "A;B;C" inside a message(); wrap it onto readable
# lines instead, since these messages exist to be read by a human in a hurry.
function(_cna_template_pretty_list items indent out_var)
    set(_line "")
    set(_text "")
    foreach(_item IN LISTS items)
        string(LENGTH "${_line}" _len)
        if(_len GREATER 62)
            string(APPEND _text "${indent}${_line},\n")
            set(_line "")
        endif()
        if(_line STREQUAL "")
            set(_line "${_item}")
        else()
            set(_line "${_line}, ${_item}")
        endif()
    endforeach()
    if(NOT _line STREQUAL "")
        string(APPEND _text "${indent}${_line}\n")
    endif()
    set(${out_var} "${_text}" PARENT_SCOPE)
endfunction()

set(CNA_TEMPLATE_MANIFEST "${CMAKE_CURRENT_LIST_DIR}/renderers.json"
    CACHE FILEPATH "Renderer metadata manifest used by cna-template")

# --- Load the manifest once into a global property -----------------------------
function(_cna_template_manifest_json out_var)
    get_property(_json GLOBAL PROPERTY _CNA_TEMPLATE_MANIFEST_JSON)
    if(NOT _json)
        if(NOT EXISTS "${CNA_TEMPLATE_MANIFEST}")
            message(FATAL_ERROR
                "cna-template: renderer manifest missing at '${CNA_TEMPLATE_MANIFEST}'.")
        endif()
        file(READ "${CNA_TEMPLATE_MANIFEST}" _json)
        set_property(GLOBAL PROPERTY _CNA_TEMPLATE_MANIFEST_JSON "${_json}")
    endif()
    set(${out_var} "${_json}" PARENT_SCOPE)
endfunction()

# --- All renderer names known to the manifest, in manifest order ---------------
function(cna_template_manifest_renderers out_var)
    _cna_template_manifest_json(_json)
    string(JSON _count LENGTH "${_json}" renderers)
    set(_names)
    math(EXPR _last "${_count} - 1")
    foreach(_i RANGE 0 ${_last})
        string(JSON _name GET "${_json}" renderers ${_i} name)
        list(APPEND _names "${_name}")
    endforeach()
    set(${out_var} "${_names}" PARENT_SCOPE)
endfunction()

# --- One metadata field for one renderer ---------------------------------------
# Unset optional fields (e.g. "webflags") come back as an empty string rather
# than aborting the configure, so the manifest can stay sparse.
function(cna_template_renderer_field renderer field out_var)
    _cna_template_manifest_json(_json)
    string(JSON _count LENGTH "${_json}" renderers)
    math(EXPR _last "${_count} - 1")
    set(${out_var} "" PARENT_SCOPE)
    foreach(_i RANGE 0 ${_last})
        string(JSON _name GET "${_json}" renderers ${_i} name)
        if(_name STREQUAL renderer)
            string(JSON _value ERROR_VARIABLE _err GET "${_json}" renderers ${_i} "${field}")
            if(_err)
                return()
            endif()
            # Arrays (platforms, webflags) become plain CMake lists.
            string(JSON _type ERROR_VARIABLE _terr TYPE "${_json}" renderers ${_i} "${field}")
            if(NOT _terr AND _type STREQUAL "ARRAY")
                string(JSON _n LENGTH "${_json}" renderers ${_i} "${field}")
                set(_items)
                if(_n GREATER 0)
                    math(EXPR _nlast "${_n} - 1")
                    foreach(_j RANGE 0 ${_nlast})
                        string(JSON _item GET "${_json}" renderers ${_i} "${field}" ${_j})
                        list(APPEND _items "${_item}")
                    endforeach()
                endif()
                set(${out_var} "${_items}" PARENT_SCOPE)
            else()
                set(${out_var} "${_value}" PARENT_SCOPE)
            endif()
            return()
        endif()
    endforeach()
endfunction()

# --- The platform token this build targets -------------------------------------
function(cna_template_current_platform out_var)
    # ANDROID must be tested before Linux: the NDK reports CMAKE_SYSTEM_NAME=Android
    # but is Linux-like enough that a naive check would misclassify it.
    if(EMSCRIPTEN)
        set(_p "web")
    elseif(ANDROID)
        set(_p "android")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(_p "windows")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(_p "macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_p "linux")
    else()
        set(_p "unknown")
    endif()
    set(${out_var} "${_p}" PARENT_SCOPE)
endfunction()

# --- CNA's own canonical renderer list, read from the checkout ------------------
# Textual, because it must work BEFORE add_subdirectory(CNA). If CNA ever
# reformats that line this degrades to the manifest list with a warning rather
# than bricking the template; cna_template_assert_no_drift() below is the
# authoritative check and still fails loudly.
function(cna_template_canonical_renderers cna_root out_var out_source)
    set(_file "${cna_root}/cmake/RendererSelection.cmake")
    set(_names)
    if(EXISTS "${_file}")
        file(STRINGS "${_file}" _line
             REGEX "^set_property\\(CACHE CNA_GRAPHICS_RENDERER PROPERTY STRINGS")
        if(_line)
            list(GET _line 0 _line)
            string(REGEX MATCHALL "\"[A-Z0-9_]+\"" _quoted "${_line}")
            foreach(_q IN LISTS _quoted)
                string(REPLACE "\"" "" _q "${_q}")
                list(APPEND _names "${_q}")
            endforeach()
        endif()
    endif()

    if(_names)
        set(${out_var} "${_names}" PARENT_SCOPE)
        set(${out_source} "cna" PARENT_SCOPE)
    else()
        cna_template_manifest_renderers(_names)
        message(WARNING
            "cna-template: could not read the canonical renderer list from\n"
            "  ${_file}\n"
            "Falling back to this template's own manifest. If CNA changed the shape of "
            "that line, update cna_template_canonical_renderers() in "
            "cmake/CnaRenderers.cmake.")
        set(${out_var} "${_names}" PARENT_SCOPE)
        set(${out_source} "manifest" PARENT_SCOPE)
    endif()
endfunction()

# --- Report a set difference in a way a human can act on -----------------------
function(_cna_template_report_drift canonical manifest)
    set(_missing "${canonical}")
    list(REMOVE_ITEM _missing ${manifest})
    set(_extra "${manifest}")
    list(REMOVE_ITEM _extra ${canonical})

    if(NOT _missing AND NOT _extra)
        return()
    endif()

    set(_msg "cna-template: renderer manifest is out of sync with CNA.\n")
    if(_missing)
        list(LENGTH _missing _n)
        string(APPEND _msg
            "\n  ${_n} renderer(s) exist in CNA but are missing from cmake/renderers.json:\n"
            "    ${_missing}\n")
    endif()
    if(_extra)
        list(LENGTH _extra _n)
        string(APPEND _msg
            "\n  ${_n} renderer(s) are listed in cmake/renderers.json but no longer exist in CNA:\n"
            "    ${_extra}\n")
    endif()
    string(APPEND _msg
        "\nAdd or remove the entries in cmake/renderers.json, then regenerate the\n"
        "presets and the renderer table:\n"
        "    python3 tools/gen_renderer_files.py\n")
    message(FATAL_ERROR "${_msg}")
endfunction()

# --- Pre-add_subdirectory validation -------------------------------------------
# Rejects unknown names and platform-invalid combinations with an actionable
# message. CNA's own gates stay authoritative; this only fires earlier and
# explains *why*, which CNA cannot always do (several of its gates are warnings,
# and a wrong name otherwise reaches CNA as a bare "Unknown graphics renderer").
function(cna_template_validate_renderer renderer cna_root)
    cna_template_canonical_renderers("${cna_root}" _canonical _source)
    cna_template_manifest_renderers(_manifest)
    _cna_template_report_drift("${_canonical}" "${_manifest}")

    list(LENGTH _canonical _count)

    if(NOT renderer IN_LIST _canonical)
        # Offer the closest matches rather than dumping 50 names unsorted.
        set(_hint)
        foreach(_r IN LISTS _canonical)
            if(_r MATCHES "^${renderer}" OR renderer MATCHES "^${_r}")
                list(APPEND _hint "${_r}")
            endif()
        endforeach()
        set(_msg
            "cna-template: unknown renderer CNA_GRAPHICS_RENDERER='${renderer}'.\n")
        if(_hint)
            string(REPLACE ";" ", " _hint_text "${_hint}")
            string(APPEND _msg "  Did you mean: ${_hint_text}?\n")
        endif()
        if(renderer STREQUAL "EASYGL")
            string(APPEND _msg
                "  EASYGL is no longer a renderer name -- it is the internal implementation\n"
                "  shared by OPENGLES2, OPENGLES3, OPENGL33, WEBGL1 and WEBGL2. Pick one of\n"
                "  those instead: OPENGLES3 for desktop Linux, WEBGL2 for the web.\n")
        endif()
        _cna_template_pretty_list("${_canonical}" "    " _all)
        string(APPEND _msg
            "  All ${_count} renderers CNA accepts:\n${_all}"
            "  docs/renderers.md says what each one is and where it runs.")
        message(FATAL_ERROR "${_msg}")
    endif()

    cna_template_current_platform(_platform)
    cna_template_renderer_field("${renderer}" platforms _platforms)
    cna_template_renderer_field("${renderer}" experimental _experimental)

    # Permitted by CNA but not supported by it: warn and continue rather than
    # blocking. The template must not be more restrictive than CNA itself.
    if(_platform IN_LIST _experimental)
        cna_template_renderer_field("${renderer}" notes _notes)
        message(WARNING
            "cna-template: '${renderer}' on ${_platform} is untested.\n"
            "  CNA does not reject this combination, but does not support it either,\n"
            "  so treat any failure as expected rather than as a template bug.\n"
            "  ${_notes}")
        return()
    endif()

    if(_platforms AND NOT _platform IN_LIST _platforms)
        cna_template_renderer_field("${renderer}" notes _notes)
        # Suggest renderers that ARE valid here, preferring ones with a preset.
        set(_ok)
        foreach(_r IN LISTS _manifest)
            cna_template_renderer_field("${_r}" platforms _rp)
            cna_template_renderer_field("${_r}" preset _rpreset)
            if(_platform IN_LIST _rp AND _rpreset)
                list(APPEND _ok "${_r}")
            endif()
        endforeach()
        string(REPLACE ";" ", " _platforms_text "${_platforms}")
        _cna_template_pretty_list("${_ok}" "    " _ok_text)

        # Where a cross-build is actually possible, say so instead of only
        # reporting that the host is wrong.
        string(TOLOWER "${renderer}" _slug)
        string(REPLACE "_" "-" _slug "${_slug}")
        cna_template_renderer_field("${renderer}" preset _has_preset)
        set(_cross "")
        if(_platforms STREQUAL "windows" AND _platform STREQUAL "linux" AND _has_preset)
            set(_cross
                "  To build it from here, cross-compile for Windows:\n"
                "    cmake --preset windows-${_slug}\n")
        elseif(_platforms STREQUAL "web" AND _has_preset)
            set(_cross
                "  To build it, activate the Emscripten SDK and use:\n"
                "    cmake --preset web-${_slug}\n")
        endif()

        message(FATAL_ERROR
            "cna-template: renderer '${renderer}' cannot target ${_platform}.\n"
            "  ${renderer} is supported on: ${_platforms_text}\n"
            "  This build targets:        ${_platform}"
            " (CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})\n"
            "  ${_notes}\n"
            ${_cross}
            "  Renderers with a ready-made preset for ${_platform}:\n${_ok_text}"
            "  Full matrix: docs/renderers.md")
    endif()
endfunction()

# --- Post-add_subdirectory drift assertion --------------------------------------
# The cache property is what CNA actually validates against, so this catches a
# drift even if the textual parse above silently read something stale.
function(cna_template_assert_no_drift)
    get_property(_cached CACHE CNA_GRAPHICS_RENDERER PROPERTY STRINGS)
    if(NOT _cached)
        return()
    endif()
    cna_template_manifest_renderers(_manifest)
    _cna_template_report_drift("${_cached}" "${_manifest}")
endfunction()
