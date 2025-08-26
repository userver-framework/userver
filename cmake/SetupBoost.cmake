include_guard(GLOBAL)

include(DownloadUsingCPM)

option(USERVER_DOWNLOAD_PACKAGE_BOOST "Download and setup Boost if no library of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
option(USERVER_FORCE_DOWNLOAD_BOOST "Download Boost even if there is an installed system package"
       ${USERVER_FORCE_DOWNLOAD_PACKAGES}
)

# if(NOT USERVER_FORCE_DOWNLOAD_BOOST)
#     if(USERVER_DOWNLOAD_PACKAGE_BOOST)
#         find_package(
#             Boost 
#             COMPONENTS program_options filesystem regex stacktrace_basic context coroutine locale iostreams
#             OPTIONAL_COMPONENTS stacktrace_backtrace stacktrace_windbg
#         )
#     else()
#         find_package(
#             Boost REQUIRED
#             COMPONENTS program_options filesystem regex stacktrace_basic context coroutine locale iostreams
#             OPTIONAL_COMPONENTS stacktrace_backtrace stacktrace_windbg
#         )
#     endif()
# 
#     if(Boost_FOUND)
#         return()
#     endif()
# endif()

cpmaddpackage(
    NAME Boost
    VERSION 1.89
    GITHUB_REPOSITORY boostorg/cmake
    GIT_TAG boost-1.89.0
    SOURCE_DIR "${CMAKE_BINARY_DIR}/Boost"
    EXCLUDE_FROM_ALL
)
include("${CMAKE_BINARY_DIR}/Boost/include/BoostFetch.cmake")

set(BOOST_INCLUDE_LIBRARIES
    program_options
    filesystem
    regex
    locale
    iostreams
    assert
    context
    coroutine
    bind
    throw_exception
    type_traits
    tokenizer
    core
    config
    iterator
    static_assert
    lexical_cast
    mpl
    function
    container
    smart_ptr
    any
    scope
    exception
    random
    preprocessor
    utility
    detail
    move
    intrusive
    type_index
    optional
    io
    predef
    system
    winapi
    integer
    thread
    fusion
    container_hash
    variant2
    mp11
    describe
    functional
    typeof
    tuple
    pool
    atomic
    dynamic_bitset
    function_types
    concept_check
    stacktrace_basic
    align
    date_time
    chrono
    numeric_convertion
    charconv
    ratio
    range
    conversion
    algorithm
    array
    unordered
)
foreach(LIBRARY ${BOOST_INCLUDE_LIBRARIES})
    boost_fetch(boostorg/${LIBRARY} TAG boost-1.89.0 EXCLUDE_FROM_ALL)
endforeach()

# set(Boost_VERSION_STRING "1.89")
# set(Boost_INCLUDE_DIR ${CMAKE_BINARY_DIR}/_deps/ CACHE PATH "" FORCE)
