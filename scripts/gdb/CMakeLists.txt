project(userver-gdb-scripts-section CXX)

if(NOT USERVER_ROOT_DIR)
    message(FATAL_ERROR "Include userver project rather than the userver/universal")
endif()

get_target_property(USERVER_UNIVERSAL_BINARY_DIR userver-universal BINARY_DIR)
set(GDB_AUTOGEN_DIR "${USERVER_UNIVERSAL_BINARY_DIR}/gdb_autogen")
set(GDB_AUTOGEN_TOOL "${CMAKE_CURRENT_SOURCE_DIR}/gen_gdb_printers.py")

set(PRETTY_PRINTERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pretty_printers")
file(GLOB_RECURSE GDB_COMMANDS ${PRETTY_PRINTERS_DIR}/*/cmd.py ${PRETTY_PRINTERS_DIR}/*/*_cmd.py)
file(GLOB_RECURSE GDB_PRETTY_PRINTERS ${PRETTY_PRINTERS_DIR}/*/printers.py ${PRETTY_PRINTERS_DIR}/*/*_printers.py)

set(USERVER_GDB_NAMESPACE "")
string(
    REGEX MATCHALL
    "namespace[ \t]+[a-zA-Z_][a-zA-Z0-9_:]*"
    USERVER_GDB_NAMESPACE_DECLARATIONS
    "${USERVER_NAMESPACE_BEGIN}"
)
foreach(USERVER_GDB_NAMESPACE_DECLARATION IN LISTS USERVER_GDB_NAMESPACE_DECLARATIONS)
    string(
        REGEX REPLACE
        "^namespace[ \t]+"
        ""
        USERVER_GDB_NAMESPACE_COMPONENT
        "${USERVER_GDB_NAMESPACE_DECLARATION}"
    )
    string(APPEND USERVER_GDB_NAMESPACE "${USERVER_GDB_NAMESPACE_COMPONENT}::")
endforeach()
if(USERVER_NAMESPACE AND NOT USERVER_GDB_NAMESPACE)
    message(
        FATAL_ERROR
            "Failed to derive the GDB namespace from USERVER_NAMESPACE_BEGIN='${USERVER_NAMESPACE_BEGIN}'. "
            "Define it using C++ namespace declarations, for example: namespace ${USERVER_NAMESPACE} {"
    )
endif()

_userver_initialize_codegen_flag()

function(_gen_gdb_file FILEGROUP GDB_CMD_SOURCE)
    file(RELATIVE_PATH CMD_REL_PATH ${PRETTY_PRINTERS_DIR}/ ${GDB_CMD_SOURCE})
    get_filename_component(CODEGEN_PATH "${CMD_REL_PATH}" DIRECTORY)
    get_filename_component(CODEGEN_FILENAME "${CMD_REL_PATH}" NAME_WLE)

    set(OUTPUT_FILE "${GDB_AUTOGEN_DIR}/${CODEGEN_PATH}/${CODEGEN_FILENAME}.cpp")
    get_filename_component(OUTPUT_FILE_DIR "${OUTPUT_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUTPUT_FILE_DIR}")

    set(GDB_NAMESPACE_ARGS)
    if(FILEGROUP STREQUAL "commands")
        list(APPEND GDB_NAMESPACE_ARGS "--userver-namespace=${USERVER_GDB_NAMESPACE}")
    endif()

    add_custom_command(
        OUTPUT "${OUTPUT_FILE}"
        COMMAND
            "${USERVER_PYTHON_PATH}" "${GDB_AUTOGEN_TOOL}" ${GDB_NAMESPACE_ARGS} "${OUTPUT_FILE}"
            "${GDB_CMD_SOURCE}"
        DEPENDS "${GDB_AUTOGEN_TOOL}" "${GDB_CMD_SOURCE}"
        COMMENT "Generating gdb ${FILEGROUP} at ${OUTPUT_FILE}" ${CODEGEN}
    )
    list(APPEND GDB_CODEGEN_SOURCES "${OUTPUT_FILE}")
    set(GDB_CODEGEN_SOURCES "${GDB_CODEGEN_SOURCES}" PARENT_SCOPE)
endfunction()

foreach(GDB_CMD_SOURCE ${GDB_COMMANDS})
    _gen_gdb_file("commands" "${GDB_CMD_SOURCE}")
endforeach()

foreach(GDB_PP_SOURCE ${GDB_PRETTY_PRINTERS})
    _gen_gdb_file("pretty printers" "${GDB_PP_SOURCE}")
endforeach()

add_library(${PROJECT_NAME} STATIC ${GDB_CODEGEN_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES USERVER_GDB_NAMESPACE "${USERVER_GDB_NAMESPACE}")
set_target_properties(userver-universal PROPERTIES USERVER_GDB_NAMESPACE "${USERVER_GDB_NAMESPACE}")
_userver_install_targets(COMPONENT universal TARGETS ${PROJECT_NAME})

function(userver_link_gdb_scripts TARGET_NAME)
    if(NOT TARGET "${TARGET_NAME}")
        message(FATAL_ERROR "Cannot add userver GDB scripts: '${TARGET_NAME}' is not a CMake target")
    endif()
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
        return()
    endif()

    target_link_libraries(
        ${TARGET_NAME}
        PRIVATE "-Wl,--whole-archive" userver::gdb-scripts-section "-Wl,--no-whole-archive"
    )
endfunction()
