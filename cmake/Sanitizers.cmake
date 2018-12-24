cmake_minimum_required (VERSION 3.8)

if (TARGET sanitize-target)
    return()
endif()

include(ValidateCRC32)

SET(SANITIZE_ENUM "mem, addr, thread, ub")
set(SANITIZE "" CACHE STRING "Clang sanitizer, possible values: ${SANITIZE_ENUM}")
if (NOT CLANG AND SANITIZE)
  message(FATAL_ERROR "-DSANITIZE can be set only when complied using clang.  Please set CC=clang-5.0 CXX=clang++-5.0 or smth.")
endif()

add_library(sanitize-target INTERFACE)

if(SANITIZE STREQUAL "")
  # no sanitizer
else()
  # New file name forces the `ccache` to rebuild on file change. That's why we have the CRC32 in the file name and its validation.
  set(USERVER_SANITIZERS_BLACKLIST "${CMAKE_CURRENT_LIST_DIR}/san_blacklist.5a0bfc44.txt")
  vaildate_crc32(${USERVER_SANITIZERS_BLACKLIST})
  target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_SANITIZERS_BLACKLIST})

  # Appending a blacklist from uservices (or other projects that set ${SANITIZE_BLACKLIST})
  if(DEFINED SANITIZE_BLACKLIST AND (NOT SANITIZE_BLACKLIST STREQUAL ""))
    vaildate_crc32(${SANITIZE_BLACKLIST})
    target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${SANITIZE_BLACKLIST})
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
