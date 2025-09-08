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
string(REGEX MATCH ^[0-9]+ USERVER_MAJOR_VERSION "${VERSION}")
string(REGEX MATCH [-0-9a-z]+$ USERVER_MINOR_VERSION "${VERSION}")

set(USERVER_VERSION "${USERVER_MAJOR_VERSION}.${USERVER_MINOR_VERSION}")
string(REPLACE "-" "_" USERVER_VERSION_STR "${USERVER_VERSION}")
string(REPLACE "." "_" USERVER_VERSION_STR "${USERVER_VERSION_STR}")
set(USERVER_VERSION_STR "v${USERVER_VERSION_STR}")
