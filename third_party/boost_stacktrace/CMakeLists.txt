project(userver-stacktrace)
include(CheckIncludeFile)

find_package(Boost REQUIRED) # Headers needed

if(NOT MACOS AND "${CMAKE_SYSTEM}" MATCHES "BSD")
    set(STACKTRACE_DEFAULT OFF)
else()
    set(STACKTRACE_DEFAULT ON)
endif()

# You can choose the implementation depending on your needs.
# See http://boostorg.github.io/stacktrace/stacktrace/configuration_and_build.html for more info.
option(USERVER_FEATURE_STACKTRACE "Allow capturing stacktraces using boost::stacktrace" ${STACKTRACE_DEFAULT})
if (NOT USERVER_FEATURE_STACKTRACE)
    set(BACKTRACE_USERVER_SOURCES src/noop.cpp)
elseif (WIN32)
    CHECK_INCLUDE_FILE(dbgeng.h HAS_DBGENG_LIBRARY)
    if(NOT HAS_DBGENG_LIBRARY)
        message(FATAL_ERROR "Failed to find <dbgeng.h>. Use -DUSERVER_FEATURE_STACKTRACE=0 or install the required packages.")
    endif()

    set(BACKTRACE_USERVER_SOURCES src/windbg_cached.cpp)
    set(BACKTRACE_USERVER_LIBRARIES ole32 dbgeng)

    #set(BACKTRACE_USERVER_SOURCES src/windbg.cpp)
    #set(BACKTRACE_USERVER_LIBRARIES ole32 dbgeng)
elseif (MACOS)
    set(BACKTRACE_USERVER_SOURCES src/basic.cpp)
    set(BACKTRACE_USERVER_LIBRARIES dl)
else()
    CHECK_INCLUDE_FILE(backtrace.h HAS_BACKTRACE_LIBRARY)
    if (NOT HAS_BACKTRACE_LIBRARY)
        # Attempt to find file in GCC specific path
        file(GLOB BacktraceIncludeFiles
	     "/usr/lib/gcc/x86_64-linux-gnu/*/include/backtrace.h"
        )
        list(SORT BacktraceIncludeFiles)
        list(GET BacktraceIncludeFiles 0 BacktraceIncludeFile)
        if (BacktraceIncludeFile)
            message(STATUS "Found ${BacktraceIncludeFile}")
        else()
            message(FATAL_ERROR "Failed to find <backtrace.h>. Use -DUSERVER_FEATURE_STACKTRACE=0 or install \
                the required packages (for Ubuntu: build-essential or libgcc-*-dev).")
        endif()
    endif()

    #set(BACKTRACE_USERVER_SOURCES src/addr2line.cpp)
    #set(BACKTRACE_USERVER_LIBRARIES dl)

    set(BACKTRACE_USERVER_SOURCES src/backtrace.cpp)
    set(BACKTRACE_USERVER_LIBRARIES backtrace dl)

    #set(BACKTRACE_USERVER_SOURCES src/basic.cpp)
    #set(BACKTRACE_USERVER_LIBRARIES dl)
endif()

add_library (${PROJECT_NAME} STATIC ${BACKTRACE_USERVER_SOURCES})
if (BacktraceIncludeFile)
    target_compile_definitions(
        ${PROJECT_NAME}
        PRIVATE BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=<${BacktraceIncludeFile}>
    )
else()
    target_compile_definitions(
        ${PROJECT_NAME} PUBLIC BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED=1
    )
endif()
target_compile_definitions(${PROJECT_NAME} INTERFACE BOOST_STACKTRACE_LINK)
target_include_directories(${PROJECT_NAME} PUBLIC include ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PUBLIC ${BACKTRACE_USERVER_LIBRARIES})
set_target_properties (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
