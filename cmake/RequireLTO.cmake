include(CheckIPOSupported)
check_ipo_supported()

option(LTO_CACHE "Use lto cache for link time optimizations" ON)
set(LTO_CACHE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/.ltocache" CACHE STRING "LTO cache folder path")
set(LTO_CACHE_SIZE_MB "6000" CACHE STRING "LTO cache size in MB")

# Sets -flto-thin for Clang and -flto for GCC for compile and link time
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

if (LTO_CACHE AND LTO_CACHE_FOLDER AND USERVER_USE_LD MATCHES "lld")
    message(STATUS "LTO_CACHE enabled, cache folder is ${LTO_CACHE_FOLDER}")
    file(MAKE_DIRECTORY ${LTO_CACHE_FOLDER})

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--thinlto-cache-dir=${LTO_CACHE_FOLDER}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--thinlto-cache-dir=${LTO_CACHE_FOLDER}")

    if (LTO_CACHE_SIZE_MB)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--thinlto-cache-policy,cache_size_bytes=${LTO_CACHE_SIZE_MB}m:cache_size=0%:prune_interval=5m")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--thinlto-cache-policy,cache_size_bytes=${LTO_CACHE_SIZE_MB}m:cache_size=0%:prune_interval=5m")
    endif()
else()
    message(STATUS "LTO_CACHE disabled")
endif()

# check_ipo_supported() may not check linkage that may fail due to wrong linker.
# Using a more complex check.
include(CheckIncludeFileCXX)
check_include_file_cxx(string LTO_SETUP_SUCCESSFUL)
if (NOT LTO_SETUP_SUCCESSFUL)
    message(FATAL_ERROR 
        "LTO fails to compile a trivial program. See error logs for info. "
        "Try specifying another linker via USERVER_USE_LD or changing the CMAKE_AR, "
        "CMAKE_RANLIB, CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS"
    )
endif()
