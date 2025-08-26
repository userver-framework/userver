include_guard(GLOBAL)

include(DownloadUsingCPM)

option(USERVER_DOWNLOAD_PACKAGE_BOOST "Download and setup Boost if no library of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
option(USERVER_FORCE_DOWNLOAD_BOOST "Download Boost even if there is an installed system package"
       ${USERVER_FORCE_DOWNLOAD_PACKAGES}
)


set(BOOST_VERSION 1.89.0)
set(BOOST_ENABLE_CMAKE ON)
set(BOOST_INCLUDE_LIBRARIES
    program_options
    filesystem
    regex
    locale
    iostreams
    context
    coroutine
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


if(FALSE)
cpmaddpackage(
    NAME Boost
    VERSION ${BOOST_VERSION}
    GITHUB_REPOSITORY boostorg/cmake
    GIT_TAG boost-${BOOST_VERSION}
    SOURCE_DIR "${CMAKE_BINARY_DIR}/Boost"
    EXCLUDE_FROM_ALL
)
include("${CMAKE_BINARY_DIR}/Boost/include/BoostFetch.cmake")

foreach(LIBRARY ${BOOST_INCLUDE_LIBRARIES})
    boost_fetch(boostorg/${LIBRARY} TAG boost-${BOOST_VERSION} EXCLUDE_FROM_ALL)
endforeach()
endif()


include(FetchContent)
cpmaddpackage(
    NAME Boost
    VERSION 1.89.0
    # URL https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.bz2
    # URL_HASH SHA256=85a33fa22621b4f314f8e85e1a5e2a9363d22e4f4992925d4bb3bc631b5a0c7a

    
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
    URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57

    # GIT_REPOSITORY https://github.com/boostorg/boost.git
    # GIT_TAG boost-${BOOST_VERSION}
    # GIT_PROGRESS TRUE   
    # GIT_SHALLOW TRUE
    OPTIONS
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_INCLUDE_LIBRARIES program_options\\\;filesystem\\\;regex\\\;locale\\\;iostreams\\\;context\\\;coroutine\\\;stacktrace\\\;uuid"
    EXCLUDE_FROM_ALL
)

find_package(
    Boost REQUIRED
    COMPONENTS ${BOOST_INCLUDE_LIBRARIES}
)
