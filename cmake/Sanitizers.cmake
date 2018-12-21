cmake_minimum_required (VERSION 3.8)

if (TARGET sanitize-target)
    return()
endif()

include(SetupEnvironment) # required for CCACHE_EXECUTABLE and CMAKE_*_COMPILER_LAUNCHER

SET(SANITIZE_ENUM "mem, addr, thread, ub")
set(SANITIZE "" CACHE STRING "Clang sanitizer, possible values: ${SANITIZE_ENUM}")
if (NOT CLANG AND SANITIZE)
  message(FATAL_ERROR "-DSANITIZE can be set only when complied using clang.  Please set CC=clang-5.0 CXX=clang++-5.0 or smth.")
endif()

add_library(sanitize-target INTERFACE)

if(SANITIZE STREQUAL "")
  # no sanitizer
else()
  # Forces the ccache to rebuild on file change
  target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${CMAKE_CURRENT_LIST_DIR}/san_blacklist.txt)

  # Appending a blacklist from uservices (or other projects that set ${SANITIZE_BLACKLIST})
  if(DEFINED SANITIZE_BLACKLIST AND (NOT SANITIZE_BLACKLIST STREQUAL ""))
    target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${SANITIZE_BLACKLIST})
  endif()

  if(DEFINED CCACHE_EXECUTABLE)
    if(DEFINED CMAKE_C_COMPILER_LAUNCHER)
      set(CMAKE_C_COMPILER_LAUNCHER CCACHE_EXTRAFILES=${CMAKE_CURRENT_LIST_DIR}/san_blacklist.txt,${SANITIZE_BLACKLIST} ${CCACHE_EXECUTABLE})
    endif()
    if(DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
      set(CMAKE_CXX_COMPILER_LAUNCHER CCACHE_EXTRAFILES=${CMAKE_CURRENT_LIST_DIR}/san_blacklist.txt,${SANITIZE_BLACKLIST} ${CCACHE_EXECUTABLE})
    endif()
  endif()

  if(SANITIZE STREQUAL "ub")
    # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    set(SANITIZE_BUILD_FLAGS -fsanitize=undefined -fno-sanitize-recover=undefined)
  elseif(SANITIZE STREQUAL "addr")
    # http://releases.llvm.org/5.0.0/tools/clang/docs/AddressSanitizer.html
    set(SANITIZE_DEFS BOOST_USE_ASAN)
    set(SANITIZE_BUILD_FLAGS -fsanitize=address -g)
    set(SANITIZE_CXX_FLAGS -fno-omit-frame-pointer)
  elseif(SANITIZE STREQUAL "thread")
    # http://releases.llvm.org/5.0.0/tools/clang/docs/ThreadSanitizer.html
    set(SANITIZE_BUILD_FLAGS -fsanitize=thread -g)
  elseif(SANITIZE STREQUAL "mem")
    # https://clang.llvm.org/docs/MemorySanitizer.html
    set(SANITIZE_BUILD_FLAGS -fsanitize=memory -g)
    set(SANITIZE_CXX_FLAGS -fno-omit-frame-pointer)
  else()
    message(FATAL_ERROR "-DSANITIZE has invalid value (${SANITIZE}), possible values: ${SANITIZE_ENUM}")
  endif()
endif()

target_compile_definitions(sanitize-target INTERFACE
  ${SANITIZE_DEFS}
)
target_compile_options(sanitize-target INTERFACE
  ${SANITIZE_BUILD_FLAGS}
  ${SANITIZE_CXX_FLAGS}
)
target_link_libraries(sanitize-target INTERFACE
  ${SANITIZE_BUILD_FLAGS}
)
