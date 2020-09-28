if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(LTO_FLAG OFF)
else()
    set(LTO_FLAG ON)
endif()
option(LTO "Use -flto=thin for link time optimizations" LTO_FLAG)
option(LTO_CACHE "Use lto cache for link time optimizations" ON)
set(LTO_CACHE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/.ltocache" CACHE STRING "LTO cache folder path")
set(LTO_CACHE_SIZE_MB "6000" CACHE STRING "LTO cache size in MB")

## Environment checks
if(LTO)
    if(CUSTOM_LD_OK)
        # ar from binutils fails to link -flto=thin with the following message:
        # ../../src/libyandex-taxi-userver.a: error adding symbols: Archive has no index; run ranlib to add one
        # In Debian/Ubuntu llvm-ar is installed with version suffix.
        # In Mac OS with brew llvm-ar is installed without version suffix
        # into /usr/local/opt/llvm/ with brew
        if (MACOS)
            set(LLVM_PATH_HINTS /usr/local/opt/llvm/bin)
        endif()
        find_program(LLVM_AR NAMES llvm-ar-7 llvm-ar HINTS ${LLVM_PATH_HINTS})
        find_program(LLVM_RANLIB NAMES llvm-ranlib-7 llvm-ranlib HINTS ${LLVM_PATH_HINTS})
        if (NOT LLVM_AR)
            set(LTO OFF)
            set(LTO_DISABLE_REASON "llvm-ar (from llvm-7) not found")
        elseif(NOT LLVM_RANLIB)
            set(LTO OFF)
            set(LTO_DISABLE_REASON "llvm-ranlib (from llvm-7) not found")
        endif()
    else(CUSTOM_LD_OK)
        set(LTO OFF)
        set(LTO_DISABLE_REASON "unsupported linker")
    endif(CUSTOM_LD_OK)
elseif(NOT LTO_FLAG AND NOT LTO)
    set(LTO_DISABLE_REASON "local build")
else(LTO)
    set(LTO_DISABLE_REASON "user request")
endif(LTO)

## Setup
if (LTO)
    message(STATUS "LTO: ${LTO}")
    set(CMAKE_AR ${LLVM_AR})
    set(CMAKE_RANLIB ${LLVM_RANLIB})

    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto=thin")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=thin")
    add_compile_options ("-flto=thin")

    message(STATUS "LTO_CACHE: ${LTO_CACHE} in folder ${LTO_CACHE_FOLDER}")
    if (LTO_CACHE AND LTO_CACHE_FOLDER AND USE_LD MATCHES "lld")
        file(MAKE_DIRECTORY ${LTO_CACHE_FOLDER})

        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--thinlto-cache-dir=${LTO_CACHE_FOLDER}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--thinlto-cache-dir=${LTO_CACHE_FOLDER}")

        if (LTO_CACHE_SIZE_MB)
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--thinlto-cache-policy,cache_size_bytes=${LTO_CACHE_SIZE_MB}m:cache_size=0%:prune_interval=5m")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--thinlto-cache-policy,cache_size_bytes=${LTO_CACHE_SIZE_MB}m:cache_size=0%:prune_interval=5m")
        endif()
    endif()
else(LTO)
    message(STATUS "LTO: ${LTO} (${LTO_DISABLE_REASON})")
endif(LTO)
