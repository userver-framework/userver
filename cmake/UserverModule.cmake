include_guard(GLOBAL)

function(userver_module MODULE)
  unset(ARG_UNPARSED_ARGUMENTS)
  set(OPTIONS)
  set(ONE_VALUE_ARGS SOURCE_DIR)
  set(MULTI_VALUE_ARGS
      IGNORE_SOURCES
      LINK_LIBRARIES
      LINK_LIBRARIES_PRIVATE
      INCLUDE_DIRS
      INCLUDE_DIRS_PRIVATE

      UTEST_DIRS
      UTEST_SOURCES
      UTEST_LINK_LIBRARIES

      DBTEST_DIRS
      DBTEST_SOURCES
      DBTEST_LINK_LIBRARIES
      DBTEST_DATABASES
      DBTEST_ENV

      UBENCH_DIRS
      UBENCH_SOURCES
      UBENCH_LINK_LIBRARIES
      UBENCH_DATABASES
      UBENCH_ENV
  )
  cmake_parse_arguments(
      ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN}
  )
  if(ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Invalid arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  ## 1. userver-${MODULE}
  file(GLOB_RECURSE SOURCES
      "${ARG_SOURCE_DIR}/src/*.cpp"
      "${ARG_SOURCE_DIR}/src/*.hpp"
      "${ARG_SOURCE_DIR}/include/*.hpp"
  )
  list(REMOVE_ITEM SOURCES ${ARG_IGNORE_SOURCES})

  list(TRANSFORM ARG_UTEST_DIRS APPEND "/*.cpp" OUTPUT_VARIABLE UTEST_DIRS_SOURCES)
  file(GLOB_RECURSE UTEST_SOURCES
      ${ARG_UTEST_SOURCES}
      ${UTEST_DIRS_SOURCES}
  )
  list(REMOVE_ITEM SOURCES ${UTEST_SOURCES})

  list(TRANSFORM ARG_DBTEST_DIRS APPEND "/*.cpp" OUTPUT_VARIABLE DBTEST_DIRS_SOURCES)
  file(GLOB_RECURSE DBTEST_SOURCES
      ${ARG_DBTEST_SOURCES}
      ${DBTEST_DIRS_SOURCES}
  )
  list(REMOVE_ITEM SOURCES ${DBTEST_SOURCES})

  list(TRANSFORM ARG_UBENCH_DIRS APPEND "/*.cpp" OUTPUT_VARIABLE UBENCH_DIRS_SOURCES)
  file(GLOB_RECURSE UBENCH_SOURCES
      ${ARG_UBENCH_SOURCES}
      ${UBENCH_DIRS_SOURCES}
  )
  list(REMOVE_ITEM SOURCES ${UBENCH_SOURCES})

  add_library(userver-${MODULE} STATIC ${SOURCES})
  set_target_properties(userver-${MODULE} PROPERTIES LINKER_LANGUAGE CXX)

  target_include_directories(
      userver-${MODULE}
      PUBLIC
      $<BUILD_INTERFACE:${ARG_SOURCE_DIR}/include>
      ${ARG_INCLUDE_DIRS}
      PRIVATE
      "${ARG_SOURCE_DIR}/src"
      ${ARG_INCLUDE_DIRS_PRIVATE}
  )
  target_link_libraries(
      userver-${MODULE}
      PUBLIC
      userver-core
      ${ARG_LINK_LIBRARIES}
      PRIVATE
      ${ARG_LINK_LIBRARIES_PRIVATE}
  )

  _userver_directory_install(
      COMPONENT ${MODULE}
      DIRECTORY "${ARG_SOURCE_DIR}/include"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
  )
  _userver_install_targets(COMPONENT ${MODULE} TARGETS userver-${MODULE})
  _userver_directory_install(
      COMPONENT ${MODULE}
      FILES "${USERVER_ROOT_DIR}/cmake/install/userver-${MODULE}-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
  )

  ## 2. userver-${MODULE}-unittest
  if(USERVER_IS_THE_ROOT_PROJECT AND UTEST_SOURCES)
    add_executable(userver-${MODULE}-unittest ${UTEST_SOURCES})
    target_link_libraries(userver-${MODULE}-unittest PRIVATE
        userver-utest
        userver-${MODULE}
        ${ARG_UTEST_LINK_LIBRARIES}
    )
    target_include_directories(userver-${MODULE}-unittest PRIVATE
        $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
        ${ARG_UTEST_DIRS}
    )
    add_google_tests(userver-${MODULE}-unittest)
  endif()

  ## 3. userver-${MODULE}-dbtest
  if(USERVER_IS_THE_ROOT_PROJECT AND DBTEST_SOURCES)
    add_executable(userver-${MODULE}-dbtest ${DBTEST_SOURCES})
    target_link_libraries(userver-${MODULE}-dbtest PRIVATE
        userver-utest
        userver-${MODULE}
        ${ARG_DBTEST_LINK_LIBRARIES}
    )
    target_include_directories(userver-${MODULE}-dbtest PRIVATE
        $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
        ${ARG_DBTEST_DIRS}
    )
    userver_add_utest(
        NAME userver-${MODULE}-dbtest
        DATABASES ${ARG_DBTEST_DATABASES}
        TEST_ENV ${ARG_DBTEST_ENV}
    )
  endif()

  ## 4. userver-${MODULE}-benchmark
  if(USERVER_IS_THE_ROOT_PROJECT AND UBENCH_SOURCES)
    add_executable(userver-${MODULE}-benchmark ${UBENCH_SOURCES})
    target_link_libraries(userver-${MODULE}-benchmark PRIVATE
        userver-ubench
        userver-${MODULE}
        ${ARG_UBENCH_LINK_LIBRARIES}
    )
    target_include_directories(userver-${MODULE}-benchmark PRIVATE
        $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
        ${ARG_UBENCH_DIRS}
    )
    userver_add_ubench_test(
        NAME userver-${MODULE}-benchmark
        DATABASES ${ARG_UBENCH_DATABASES}
        TEST_ENV ${ARG_UBENCH_ENV}
    )
  endif()
endfunction()
