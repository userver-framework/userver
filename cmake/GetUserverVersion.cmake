string(TIMESTAMP USERVER_BUILD_TIME %Y%m%d%H%M%S UTC)

set(USERVER_HASH "unknown")

find_package(Git)

if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE USERVER_HASH
        RESULT_VARIABLE STATUS
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(STATUS)
        set(USERVER_HASH "unknown")
        message(STATUS "Failed to retrieve git short hash")
    endif()
else()
    message(STATUS "Git not found")
endif()

file(READ ${USERVER_ROOT_DIR}/version.txt VERSION)
string(STRIP ${VERSION} VERSION)
string(REGEX MATCH "^([0-9]+)\.([0-9]+[\-a-z0-9]*)" USERVER_MATCHED_VERSION_STRING "${VERSION}")
if(NOT USERVER_MATCHED_VERSION_STRING STREQUAL VERSION)
    message(FATAL_ERROR "Failed to retrieve userver major/minor version number from '${VERSION}'")
endif()
set(USERVER_MAJOR_VERSION ${CMAKE_MATCH_1})
set(USERVER_MINOR_VERSION ${CMAKE_MATCH_2})

set(USERVER_VERSION "${USERVER_MAJOR_VERSION}.${USERVER_MINOR_VERSION}")
string(REPLACE "-" "_" USERVER_VERSION_STR "${USERVER_VERSION}")
string(REPLACE "." "_" USERVER_VERSION_STR "${USERVER_VERSION_STR}")
set(USERVER_VERSION_STR "v${USERVER_VERSION_STR}")
