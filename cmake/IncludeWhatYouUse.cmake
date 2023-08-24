option(IWYU_ENABLED "Check with include-what-you-use" OFF)

if (IWYU_ENABLED)
  find_program(IWYU_PATH NAMES include-what-you-use iwyu REQUIRED)
  execute_process(COMMAND "${IWYU_PATH}" --version
    OUTPUT_VARIABLE IWYU_INFO
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(IWYU_INFO)
  list(GET IWYU_INFO 1 IWYU_VERSION)

  set(IWYU_VERSION_REQUIRED "0.13")
  if (IWYU_VERSION VERSION_LESS IWYU_VERSION_REQUIRED)
    message(FATAL_ERROR "IWYU ${IWYU_PATH} version too low (${IWYU_VERSION}), must be at least ${IWYU_VERSION_REQUIRED}")
  endif()

  set(IWYU_ARGS
    "-Xiwyu;--cxx17ns"
    "-Xiwyu;--no_comments"
    "-Xiwyu;--no_fwd_decls"
  )
  message(STATUS "Found IWYU: ${IWYU_PATH}, arguments ${IWYU_ARGS}")
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH} ${IWYU_ARGS})
endif()
