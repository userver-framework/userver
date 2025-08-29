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
    atomic
    program_options
    filesystem
    regex
    locale
    iostreams
    context
    coroutine
    stacktrace
    coroutine2

    uuid
    lockfree
    endian
)
string(REGEX REPLACE ";" "\\\\\\\\;" BOOST_INCLUDE_LIBRARIES_LIST "${BOOST_INCLUDE_LIBRARIES}")

if(NOT USERVER_FORCE_DOWNLOAD_BOOST AND NOT BOOST_CPM)
    if(USERVER_DOWNLOAD_PACKAGE_BOOST)
        set(MAYBE_REQUIRED)
    else()
        set(MAYBE_REQUIRED REQUIRED)
    endif()
    find_package(
        Boost CONFIG ${MAYBE_REQUIRED}
        COMPONENTS program_options filesystem regex stacktrace_basic context coroutine locale iostreams
        OPTIONAL_COMPONENTS stacktrace_backtrace stacktrace_windbg
    )

    if(Boost_FOUND)
        return()
    endif()
endif()
set(BOOST_CPM TRUE CACHE BOOL "")

cpmaddpackage(
    NAME Boost
    VERSION ${BOOST_VERSION}
    URL https://github.com/boostorg/boost/releases/download/boost-${BOOST_VERSION}/boost-${BOOST_VERSION}-cmake.tar.xz
    URL_HASH SHA256=67acec02d0d118b5de9eb441f5fb707b3a1cdd884be00ca24b9a73c995511f74
    OPTIONS
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_INCLUDE_LIBRARIES ${BOOST_INCLUDE_LIBRARIES_LIST}"
        "BOOST_SKIP_INSTALL_RULES ON"
	"BUILD_SHARED_LIBS OFF"
	"BOOST_RUNTIME_LINK static"
    EXCLUDE_FROM_ALL
)

# We have fresh version of boost, DWCAS should work
set(USERVER_IMPL_DWCAS_CHECKED TRUE CACHE INTERNAL "TRUE iff checked that DWCAS works")
