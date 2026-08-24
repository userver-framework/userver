_userver_module_begin(
    NAME lz4
    # Upstream lz4 CMake creates a target named "lz4"; keep that name free so CPM
    # and check_library_exists(lz4 ...) (CMake 4.x / Snappy) do not clash.
    TARGET_NAME lz4::lz4
    VERSION 1.9.2
    DEBIAN_NAMES liblz4-dev
    FORMULA_NAMES lz4
    PKG_CONFIG_NAMES liblz4
    CPM_NAME lz4
    CPM_VERSION 1.10.0
    CPM_GITHUB_REPOSITORY lz4/lz4
    CPM_GIT_TAG v1.10.0
    CPM_SOURCE_SUBDIR build/cmake
    CPM_OPTIONS "LZ4_BUILD_CLI OFF" "LZ4_BUILD_LEGACY_LZ4C OFF" "BUILD_SHARED_LIBS OFF" "BUILD_STATIC_LIBS ON"
)

_userver_module_find_include(NAMES lz4.h)

_userver_module_find_library(NAMES lz4)

_userver_module_end()

if(lz4_ADDED)
    # Prefer the static library produced by lz4's build/cmake.
    if(TARGET lz4_static)
        set(_userver_lz4_real lz4_static)
    elseif(TARGET lz4)
        set(_userver_lz4_real lz4)
    else()
        message(FATAL_ERROR "lz4 cmake target not found after CPM download, don't know how to link")
    endif()
    if(NOT TARGET lz4::lz4)
        add_library(lz4::lz4 ALIAS ${_userver_lz4_real})
    endif()
    if(NOT TARGET LZ4::LZ4)
        add_library(LZ4::LZ4 ALIAS ${_userver_lz4_real})
    endif()
    unset(_userver_lz4_real)
else()
    if(NOT TARGET LZ4::LZ4)
        add_library(LZ4::LZ4 ALIAS lz4::lz4)
    endif()
endif()
