include_guard(GLOBAL)

include(Directories)

function(_userver_get_sanitize_options SANITIZER_LIST_VAR COMPILE_FLAGS_VAR LINK_FLAGS_VAR)
  _get_userver_cmake_dir(USERVER_CMAKE_DIR)

  set(USERVER_SANITIZE_ENUM "mem, addr, thread, ub")

  set(USERVER_SANITIZE "" CACHE STRING "Sanitizer, possible values: ${USERVER_SANITIZE_ENUM}")
  set(USERVER_SANITIZE_BLACKLIST "" CACHE FILEPATH "Blacklist for sanitizers")

  set(sanitizers_supported ON)
  if (USERVER_SANITIZE AND
      CMAKE_SYSTEM_NAME MATCHES "Darwin" AND
      CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64")
    message(WARNING
        "Sanitizers on aarch64 MacOS produce false positive "
        "on coroutine-context switching. Disabling")
    set(sanitizers_supported OFF)
  endif()

  set(sanitize_cxx_and_link_flags "-g")
  set(sanitize_cxx_flags)
  set(sanitize_link_flags)
  set(sanitizer_list)

  if (USERVER_SANITIZE AND sanitizers_supported)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      set(USERVER_BLACKLIST "${USERVER_CMAKE_DIR}/sanitize.blacklist.txt")
      list(APPEND sanitize_cxx_flags "-fsanitize-blacklist=${USERVER_BLACKLIST}")
      list(APPEND sanitize_link_flags "-fsanitize-blacklist=${USERVER_BLACKLIST}")

      set(USERVER_MACOS_BLACKLIST "${USERVER_CMAKE_DIR}/sanitize-macos.blacklist.txt")
      if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        list(APPEND sanitize_cxx_flags "-fsanitize-blacklist=${USERVER_MACOS_BLACKLIST}")
        list(APPEND sanitize_link_flags "-fsanitize-blacklist=${USERVER_MACOS_BLACKLIST}")
      endif()

      if (USERVER_SANITIZE_BLACKLIST)
        list(APPEND sanitize_cxx_flags "-fsanitize-blacklist=${USERVER_SANITIZE_BLACKLIST}")
        list(APPEND sanitize_link_flags "-fsanitize-blacklist=${USERVER_SANITIZE_BLACKLIST}")
      endif()
    endif()

    set(sanitizer_list ${USERVER_SANITIZE})
    separate_arguments(sanitizer_list)
    list(REMOVE_DUPLICATES sanitizer_list)
    list(LENGTH sanitizer_list sanitizer_count)

    foreach(sanitizer IN LISTS sanitizer_list)
      if (sanitizer STREQUAL "thread")
        # https://clang.llvm.org/docs/ThreadSanitizer.html
        if (sanitizer_count GREATER 1)
          message(WARNING "ThreadSanitizer should not be combined with other sanitizers")
        endif()
        list(APPEND sanitize_cxx_and_link_flags -fsanitize=thread)

      elseif (sanitizer STREQUAL "ub")
        # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
        list(APPEND sanitize_cxx_and_link_flags -fsanitize=undefined -fno-sanitize-recover=undefined)

      elseif (sanitizer STREQUAL "addr")
        # https://clang.llvm.org/docs/AddressSanitizer.html
        list(APPEND sanitize_cxx_and_link_flags -fsanitize=address)
        if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
          # gcc links with ASAN dynamically by default, and that leads to all sorts of problems
          # when we intercept dl_iterate_phdr, which ASAN uses in initialization before main.
          list(APPEND sanitize_cxx_and_link_flags -static-libasan)
        endif()
        list(APPEND sanitize_cxx_flags -fno-omit-frame-pointer)

      elseif (sanitizer STREQUAL "mem")
        # https://clang.llvm.org/docs/MemorySanitizer.html
        list(APPEND sanitize_cxx_and_link_flags -fsanitize=memory)
        list(APPEND sanitize_cxx_flags -fno-omit-frame-pointer)

      else()
        message(FATAL_ERROR
            "-DUSERVER_SANITIZE has invalid value(s) (${SANITIZE_PENDING}), "
            "possible values: ${USERVER_SANITIZE_ENUM}")
      endif()
    endforeach()

    message(STATUS "Sanitizers are ON: ${USERVER_SANITIZE}")
  endif()

  set("${SANITIZER_LIST_VAR}" ${sanitizer_list} PARENT_SCOPE)
  set("${COMPILE_FLAGS_VAR}" ${sanitize_cxx_flags} ${sanitize_cxx_and_link_flags} PARENT_SCOPE)
  set("${LINK_FLAGS_VAR}" ${sanitize_link_flags} ${sanitize_cxx_and_link_flags} PARENT_SCOPE)
endfunction()

function(_userver_make_sanitize_target)
  if (TARGET userver-internal-sanitize-options)
    return()
  endif()

  _userver_get_sanitize_options(sanitizer_list sanitize_cxx_flags sanitize_link_flags)
  add_library(userver-internal-sanitize-options INTERFACE)
  target_compile_options(userver-internal-sanitize-options INTERFACE
      ${sanitize_cxx_flags}
  )
  target_link_libraries(userver-internal-sanitize-options INTERFACE
      ${sanitize_link_flags}
  )
endfunction()
