include_guard()

include(UserverTestsuite)

set(CHAOTIC_BIN ${CMAKE_CURRENT_LIST_DIR}/../chaotic/bin/chaotic-gen)
message(STATUS "Found chaotic-gen: ${CHAOTIC_BIN}")

userver_venv_setup(
    NAME userver-chaotic
    PYTHON_OUTPUT_VAR USERVER_CHAOTIC_PYTHON_BINARY
    REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/chaotic/requirements.txt"
    UNIQUE
)

function(userver_target_generate_chaotic TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS OUTPUT_DIR RELATIVE_TO)
    set(MULTI_VALUE_ARGS SCHEMAS ARGS)
    cmake_parse_arguments(
        PARSE "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN}
    )

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
            ${PARSE_ARGS}
            -o "${PARSE_OUTPUT_DIR}"
	    --relative-to "${RELATIVE_TO}"
            ${PARSE_SCHEMAS}
        DEPENDS
            ${PARSE_SCHEMAS}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
    add_library(${TARGET} ${SCHEMAS})
    target_link_libraries(${TARGET} userver-universal userver-chaotic)
    target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
    target_include_directories(${TARGET} PUBLIC "${PARSE_OUTPUT_DIR}")
endfunction()
