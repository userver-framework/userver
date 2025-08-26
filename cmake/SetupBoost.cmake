include_guard(GLOBAL)

include(DownloadUsingCPM)

option(USERVER_DOWNLOAD_PACKAGE_BOOST "Download and setup Boost if no library of matching version was found"
       ${USERVER_DOWNLOAD_PACKAGES}
)
option(USERVER_FORCE_DOWNLOAD_BOOST "Download Boost even if there is an installed system package"
       ${USERVER_FORCE_DOWNLOAD_PACKAGES}
)


set(BOOST_VERSION 1.89.0)
set(BOOST_INCLUDE_LIBRARIES
    program_options
    filesystem
    regex
    locale
    iostreams
    context
    coroutine
)

if(NOT USERVER_FORCE_DOWNLOAD_BOOST)
    if(USERVER_DOWNLOAD_PACKAGE_BOOST)
        find_package(
            Boost 
            COMPONENTS program_options filesystem regex stacktrace_basic context coroutine locale iostreams
            OPTIONAL_COMPONENTS stacktrace_backtrace stacktrace_windbg
        )
    else()
        find_package(
            Boost REQUIRED
            COMPONENTS program_options filesystem regex stacktrace_basic context coroutine locale iostreams
            OPTIONAL_COMPONENTS stacktrace_backtrace stacktrace_windbg
        )
    endif()

    if(Boost_FOUND)
        return()
    endif()
endif()

cpmaddpackage(
    NAME Boost
    VERSION ${BOOST_VERSION}
    URL https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}-cmake.tar.xz
    URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
    OPTIONS
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_INCLUDE_LIBRARIES program_options\\\;filesystem\\\;regex\\\;locale\\\;iostreams\\\;context\\\;coroutine\\\;stacktrace\\\;uuid\\\;coroutine2"
    EXCLUDE_FROM_ALL
)

find_package(
    Boost REQUIRED
    COMPONENTS ${BOOST_INCLUDE_LIBRARIES}
)
