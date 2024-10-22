include_guard(GLOBAL)

function(userver_module MODULE)
  set(OPTIONS)
  set(ONE_VALUE_ARGS SOURCE_DIR)
  set(MULTI_VALUE_ARGS
      IGNORE_SOURCES
      LINK_LIBRARIES
      LINK_LIBRARIES_PRIVATE
      INCLUDE_DIRS
      INCLUDE_DIRS_PRIVATE

      UTEST_SOURCES
      UTEST_LINK_LIBRARIES
      DBTEST_SOURCES
      DBTEST_DATABASES
      UBENCH_LINK_LIBRARIES
  )
  cmake_parse_arguments(
      ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN}
  )

  ## 1. userver-${MODULE}
  file(GLOB_RECURSE
      SOURCES
          ${ARG_SOURCE_DIR}/src/*.cpp
          ${ARG_SOURCE_DIR}/src/*.hpp
          ${ARG_SOURCE_DIR}/include/*.hpp
  )
  list(REMOVE_ITEM SOURCES ${ARG_IGNORE_SOURCES})

  file(GLOB_RECURSE UNIT_TEST_SOURCES
      ${ARG_SOURCE_DIR}/*_test.cpp
      ${ARG_SOURCE_DIR}/tests/*.cpp
  )
  list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

  file(GLOB_RECURSE DBTEST_SOURCES
      ${ARG_DBTEST_SOURCES}
  )
  list(REMOVE_ITEM SOURCES ${DBTEST_SOURCES})
  list(REMOVE_ITEM UNIT_TEST_SOURCES ${DBTEST_SOURCES})

  add_library(userver-${MODULE} STATIC ${SOURCES})
  set_target_properties(userver-${MODULE} PROPERTIES LINKER_LANGUAGE CXX)

  target_include_directories(
    userver-${MODULE}
    PUBLIC
        $<BUILD_INTERFACE:${ARG_SOURCE_DIR}/include>
    PRIVATE
        ${ARG_SOURCE_DIR}/src
  )

  target_link_libraries(userver-${MODULE} PUBLIC userver-core)
  if(ARG_LINK_LIBRARIES)
    target_link_libraries(
        userver-${MODULE}
      PUBLIC
        userver-core
        ${ARG_LINK_LIBRARIES}
    )
  endif()
  if(ARG_LINK_LIBRARIES_PRIVATE)
    target_link_libraries(userver-${MODULE} PRIVATE ${ARG_LINK_LIBRARIES_PRIVATE})
  endif()
  if(ARG_INCLUDE_DIRS)
    target_include_directories(userver-${MODULE} PUBLIC ${ARG_INCLUDE_DIRS})
  endif()
  if(ARG_INCLUDE_DIRS_PRIVATE)
    target_include_directories(userver-${MODULE} PRIVATE ${ARG_INCLUDE_DIRS_PRIVATE})
  endif()

  _userver_directory_install(COMPONENT ${MODULE}
    DIRECTORY ${ARG_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
  )
  _userver_install_targets(COMPONENT ${MODULE} TARGETS userver-${MODULE})
  _userver_directory_install(
    COMPONENT
      ${MODULE}
    FILES
      "${USERVER_ROOT_DIR}/cmake/install/userver-${MODULE}-config.cmake"
    DESTINATION
      ${CMAKE_INSTALL_LIBDIR}/cmake/userver
  )

  ## 2. userver-${MODULE}-benchmark
  file(GLOB_RECURSE BENCH_SOURCES
    ${ARG_SOURCE_DIR}/benchmark/*.cpp
    ${ARG_SOURCE_DIR}/benchmark/*.hpp
  )

  if(USERVER_IS_THE_ROOT_PROJECT AND BENCH_SOURCES)
    add_executable(userver-${MODULE}-benchmark ${BENCH_SOURCES})
    target_link_libraries(userver-${MODULE}-benchmark
      userver-ubench
      userver-${MODULE}
      ${ARG_UBENCH_LINK_LIBRARIES}
    )
    target_include_directories(userver-${MODULE}-benchmark PRIVATE
      $<TARGET_PROPERTY:userver-${MODULE},INCLUDE_DIRECTORIES>
    )
    add_test(userver-${MODULE}-benchmark
      env
        ${CMAKE_BINARY_DIR}/testsuite/env
        run --
        ${CMAKE_CURRENT_BINARY_DIR}/userver-${MODULE}-benchmark
        --benchmark_min_time=0
        --benchmark_color=no
    )
  endif()

  ## 3. userver-${MODULE}-unittest
  if(USERVER_IS_THE_ROOT_PROJECT AND UNIT_TEST_SOURCES)
    add_executable(userver-${MODULE}-unittest ${UNIT_TEST_SOURCES})
    target_link_libraries(userver-${MODULE}-unittest
      userver-utest
      userver-${MODULE}
      ${ARG_UTEST_LINK_LIBRARIES}
    )
    target_include_directories(
      userver-${MODULE}-unittest
      PUBLIC
        ${ARG_SOURCE_DIR}/include
      PRIVATE
        ${ARG_SOURCE_DIR}/src
    )
    target_include_directories (userver-${MODULE}-unittest PRIVATE
      $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
    )
    add_google_tests(userver-${MODULE}-unittest)
  endif()

  ## 4. userver-${MODULE}-dbtest
  if(USERVER_IS_THE_ROOT_PROJECT AND DBTEST_SOURCES)
    add_executable(userver-${MODULE}-dbtest ${DBTEST_SOURCES})
    target_link_libraries(userver-${MODULE}-dbtest
      userver-utest
      userver-${MODULE}
      ${ARG_UTEST_LINK_LIBRARIES}
    )
    target_include_directories(
      userver-${MODULE}-dbtest
      PUBLIC
        ${ARG_SOURCE_DIR}/include
      PRIVATE
        ${ARG_SOURCE_DIR}/src
    )
    target_include_directories (userver-${MODULE}-dbtest PRIVATE
      $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
    )
    userver_add_utest(
      NAME
        userver-${MODULE}-dbtest
      DATABASES
        ${ARG_UTEST_DATABASES}
    )
  endif()
endfunction()
