# Implementation note: public functions here should be usable even without
# a direct include of this script, so the functions should not rely
# on non-cache variables being present.

include_guard(GLOBAL)

function(_userver_prepare_chaotic)
  include("${CMAKE_CURRENT_LIST_DIR}/UserverTestsuite.cmake")

  find_program(CHAOTIC_BIN chaotic-gen
      PATHS
          "${CMAKE_CURRENT_SOURCE_DIR}/../chaotic/bin"
          "${CMAKE_CURRENT_LIST_DIR}/../../../bin"
      NO_DEFAULT_PATH
  )
  message(STATUS "Found chaotic-gen: ${CHAOTIC_BIN}")
  set_property(GLOBAL PROPERTY userver_chaotic_bin "${CHAOTIC_BIN}")

  if(NOT USERVER_CHAOTIC_SCRIPTS_PATH)
    get_filename_component(USERVER_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
    set(USERVER_CHAOTIC_SCRIPTS_PATH "${USERVER_DIR}/scripts/chaotic")
  endif()

  userver_venv_setup(
      NAME userver-chaotic
      PYTHON_OUTPUT_VAR USERVER_CHAOTIC_PYTHON_BINARY
      REQUIREMENTS "${USERVER_CHAOTIC_SCRIPTS_PATH}/requirements.txt"
      UNIQUE
  )
  set_property(GLOBAL PROPERTY userver_chaotic_python_binary
      "${USERVER_CHAOTIC_PYTHON_BINARY}")
endfunction()

_userver_prepare_chaotic()

function(userver_target_generate_chaotic TARGET)
  set(OPTIONS)
  set(ONE_VALUE_ARGS OUTPUT_DIR RELATIVE_TO)
  set(MULTI_VALUE_ARGS SCHEMAS ARGS)
  cmake_parse_arguments(
      PARSE "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN}
  )

  get_property(CHAOTIC_BIN GLOBAL PROPERTY userver_chaotic_bin)
  get_property(CHAOTIC_EXTRA_ARGS GLOBAL PROPERTY userver_chaotic_extra_args)
  get_property(USERVER_CHAOTIC_PYTHON_BINARY
      GLOBAL PROPERTY userver_chaotic_python_binary)

  if (NOT DEFINED PARSE_OUTPUT_DIR)
    set(PARSE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  endif()
  file(MAKE_DIRECTORY "${PARSE_OUTPUT_DIR}")

  if (NOT PARSE_SCHEMAS)
    message(FATAL_ERROR "SCHEMAS is required")
  endif()

  if (NOT PARSE_RELATIVE_TO)
    message(FATAL_ERROR "RELATIVE_TO is required")
  endif()

  set(SCHEMAS)
  foreach(PARSE_SCHEMA ${PARSE_SCHEMAS})
    file(RELATIVE_PATH SCHEMA "${PARSE_RELATIVE_TO}" "${PARSE_SCHEMA}")

    string(REGEX REPLACE "^(.*)\\.([^.]*)\$" "\\1" SCHEMA ${SCHEMA})
    set(SCHEMA ${PARSE_OUTPUT_DIR}/${SCHEMA}.cpp)

    list(APPEND SCHEMAS ${SCHEMA})
  endforeach()

  add_custom_command(
      OUTPUT
          ${SCHEMAS}
      COMMAND
          env USERVER_PYTHON=${USERVER_CHAOTIC_PYTHON_BINARY}
          ${CHAOTIC_BIN}
          ${CHAOTIC_EXTRA_ARGS}
          ${PARSE_ARGS}
          -o "${PARSE_OUTPUT_DIR}"
          --relative-to "${PARSE_RELATIVE_TO}"
          ${PARSE_SCHEMAS}
      DEPENDS
          ${PARSE_SCHEMAS}
      WORKING_DIRECTORY
          ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
  )
  add_library(${TARGET} ${SCHEMAS})
  target_link_libraries(${TARGET} userver::chaotic)
  target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
  target_include_directories(${TARGET} PUBLIC "${PARSE_OUTPUT_DIR}")
endfunction()
