cmake_minimum_required (VERSION 3.8)

if (TARGET sanitize-target)
    return()
endif()

include(SetupEnvironment) # required for CCACHE_EXECUTABLE and CMAKE_*_COMPILER_LAUNCHER

SET(SANITIZE_ENUM "mem, addr, thread, ub")
set(SANITIZE "" CACHE STRING "Clang sanitizer, possible values: ${SANITIZE_ENUM}")
if (NOT CLANG AND SANITIZE)
  message(FATAL_ERROR "-DSANITIZE can be set only when complied using clang.  Please set CC=clang-7 CXX=clang++-7 or smth.")
endif()

add_library(sanitize-target INTERFACE)

if(SANITIZE)
  set(USERVER_BLACKLIST ${CMAKE_CURRENT_LIST_DIR}/sanitize.blacklist.txt)
  target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_BLACKLIST})
  target_link_libraries(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_BLACKLIST})

  # Appending a blacklist from uservices (or other projects that set ${SANITIZE_BLACKLIST})
  if(DEFINED SANITIZE_BLACKLIST AND (NOT SANITIZE_BLACKLIST STREQUAL ""))
    target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${SANITIZE_BLACKLIST})
    target_link_libraries(sanitize-target INTERFACE -fsanitize-blacklist=${SANITIZE_BLACKLIST})
  endif()

  set(SANITIZE_PENDING ${SANITIZE})
  separate_arguments(SANITIZE_PENDING)
  list(REMOVE_DUPLICATES SANITIZE_PENDING)

  set(SANITIZE_BUILD_FLAGS "-g")

  # should go first to for combination check
  if("thread" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "thread")
    if (SANITIZE_PENDING)
      message(WARNING "ThreadSanitizer should not be combined with other sanitizers")
    endif()

    # https://clang.llvm.org/docs/ThreadSanitizer.html
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=thread)
  endif()

  if("ub" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "ub")

    # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=undefined -fno-sanitize-recover=undefined)
  endif()

  if("addr" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "addr")

    # https://clang.llvm.org/docs/AddressSanitizer.html
    set(SANITIZE_ASAN_ENABLED ON)
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=address)
    set(SANITIZE_CXX_FLAGS -fno-omit-frame-pointer)
  endif()

  if("mem" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "mem")

    # https://clang.llvm.org/docs/MemorySanitizer.html
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=memory)
    set(SANITIZE_CXX_FLAGS -fno-omit-frame-pointer)
  endif()

  if (SANITIZE_PENDING)
    message(FATAL_ERROR "-DSANITIZE has invalid value(s) (${SANITIZE_PENDING}), possible values: ${SANITIZE_ENUM}")
  endif()
endif()

target_compile_options(sanitize-target INTERFACE
  ${SANITIZE_BUILD_FLAGS}
  ${SANITIZE_CXX_FLAGS}
)
target_link_libraries(sanitize-target INTERFACE
  ${SANITIZE_BUILD_FLAGS}
)
