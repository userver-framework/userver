project(userver-chaotic CXX)

include(ChaoticGen)

file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC userver-universal)

if(USERVER_IS_THE_ROOT_PROJECT)
  userver_venv_setup(
      NAME userver-chaotic-tests
      PYTHON_OUTPUT_VAR USERVER_CHAOTIC_PYTEST_PYTHON_BINARY
      REQUIREMENTS
          "${USERVER_ROOT_DIR}/scripts/chaotic/requirements.txt"
          "${USERVER_ROOT_DIR}/testsuite/requirements.txt"
      UNIQUE
  )
  add_test(
      NAME chaotic-tests-pytest
      COMMAND
      env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
      ${USERVER_CHAOTIC_PYTEST_PYTHON_BINARY} -m pytest
      -vvs
      ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )

  add_subdirectory(integration_tests)
  add_subdirectory(golden_tests)
endif()

_userver_directory_install(COMPONENT chaotic
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/..
)
_userver_directory_install(COMPONENT chaotic
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/chaotic
    DESTINATION 
        ${CMAKE_INSTALL_LIBDIR}/userver/
)
_userver_directory_install(COMPONENT chaotic
    PROGRAMS
        "${CMAKE_CURRENT_SOURCE_DIR}/bin/chaotic-gen"
    DESTINATION
        ${CMAKE_INSTALL_BINDIR}/
)
_userver_install_targets(COMPONENT chaotic
    TARGETS
        ${PROJECT_NAME}
)
_userver_directory_install(COMPONENT chaotic
    FILES
        "${USERVER_ROOT_DIR}/cmake/ChaoticGen.cmake"
        "${USERVER_ROOT_DIR}/cmake/install/userver-chaotic-config.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)
_userver_directory_install(COMPONENT chaotic
    DIRECTORY "${USERVER_ROOT_DIR}/scripts/chaotic"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)
