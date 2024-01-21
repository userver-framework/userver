include_guard()

set(CHAOTIC_BIN ${CMAKE_CURRENT_SOURCE_DIR}/../bin/chaotic-gen)
message(STATUS "Found chaotic-gen: ${CHAOTIC_BIN}")

function(userver_target_generate_chaotic TARGET)
    set(OPTIONS)
    set(ONE_VALUE_ARGS OUTPUT_DIR)
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

    add_custom_command(
        OUTPUT
            "${PARSE_OUTPUT_DIR}/file.cpp"
            "${PARSE_OUTPUT_DIR}/file.hpp"
        COMMAND
            ${CHAOTIC_BIN} ${PARSE_ARGS} -o "${PARSE_OUTPUT_DIR}" ${PARSE_SCHEMAS}
        DEPENDS
            ${PARSE_SCHEMAS}
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
    add_library(${TARGET} ${PARSE_OUTPUT_DIR}/file.cpp)
    target_link_libraries(${TARGET} userver-universal)
    target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")
    target_include_directories(${TARGET} PUBLIC "${PARSE_OUTPUT_DIR}")
endfunction()
