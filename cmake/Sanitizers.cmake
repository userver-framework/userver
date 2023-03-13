if (TARGET sanitize-target)
    return()
endif()

set(USERVER_SANITIZE_ENUM "mem, addr, thread, ub")

set(USERVER_SANITIZE "" CACHE STRING "Sanitizer, possible values: ${USERVER_SANITIZE_ENUM}")

add_library(sanitize-target INTERFACE)

if (USERVER_SANITIZE)
  if (CLANG)
    set(USERVER_BLACKLIST ${CMAKE_CURRENT_LIST_DIR}/sanitize.blacklist.txt)
    target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_BLACKLIST})
    target_link_libraries(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_BLACKLIST})

    set(USERVER_MACOS_BLACKLIST ${CMAKE_CURRENT_LIST_DIR}/sanitize-macos.blacklist.txt)
    if (MACOS)
      target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_MACOS_BLACKLIST})
      target_link_libraries(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_MACOS_BLACKLIST})
    endif()

    # Appending a blacklist from uservices (or other projects that set ${USERVER_SANITIZE_BLACKLIST})
    if (DEFINED SANITIZE_BLACKLIST AND (NOT SANITIZE_BLACKLIST STREQUAL ""))
      target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${SANITIZE_BLACKLIST})
      target_link_libraries(sanitize-target INTERFACE -fsanitize-blacklist=${SANITIZE_BLACKLIST})
    endif()
    if (DEFINED USERVER_SANITIZE_BLACKLIST AND (NOT USERVER_SANITIZE_BLACKLIST STREQUAL ""))
      target_compile_options(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_SANITIZE_BLACKLIST})
      target_link_libraries(sanitize-target INTERFACE -fsanitize-blacklist=${USERVER_SANITIZE_BLACKLIST})
    endif()
  endif()

  set(SANITIZE_PENDING ${USERVER_SANITIZE})
  separate_arguments(SANITIZE_PENDING)
  list(REMOVE_DUPLICATES SANITIZE_PENDING)

  set(SANITIZE_BUILD_FLAGS "-g")

  # should go first to for combination check
  if ("thread" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "thread")
    if (SANITIZE_PENDING)
      message(WARNING "ThreadSanitizer should not be combined with other sanitizers")
    endif()

    # https://clang.llvm.org/docs/ThreadSanitizer.html
    set(SANITIZE_TSAN_ENABLED ON)
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=thread)
  endif()

  if ("ub" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "ub")

    # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=undefined -fno-sanitize-recover=undefined)
  endif()

  if ("addr" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "addr")

    # https://clang.llvm.org/docs/AddressSanitizer.html
    set(SANITIZE_ASAN_ENABLED ON)
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=address)
    set(SANITIZE_CXX_FLAGS -fno-omit-frame-pointer)
  endif()

  if ("mem" IN_LIST SANITIZE_PENDING)
    list(REMOVE_ITEM SANITIZE_PENDING "mem")

    # https://clang.llvm.org/docs/MemorySanitizer.html
    set(SANITIZE_BUILD_FLAGS ${SANITIZE_BUILD_FLAGS} -fsanitize=memory)
    set(SANITIZE_CXX_FLAGS -fno-omit-frame-pointer)
  endif()

  if (SANITIZE_PENDING)
    message(FATAL_ERROR "-DUSERVER_SANITIZE has invalid value(s) (${SANITIZE_PENDING}), possible values: ${SANITIZE_ENUM}")
  endif()
  
  message(STATUS "Sanitizers are ON: ${USERVER_SANITIZE}")
endif()

target_compile_options(sanitize-target INTERFACE
  ${SANITIZE_BUILD_FLAGS}
  ${SANITIZE_CXX_FLAGS}
)
target_link_libraries(sanitize-target INTERFACE
  ${SANITIZE_BUILD_FLAGS}
)
