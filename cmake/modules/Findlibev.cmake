_userver_module_begin(
    NAME
    libev
    DEBIAN_NAMES
    libev-dev
    FORMULA_NAMES
    libev
    RPM_NAMES
    libev-devel
    PACMAN_NAMES
    libev

    CPM_NAME libev
    CPM_URL http://dist.schmorp.de/libev/libev-4.33.tar.gz
    CPM_DOWNLOAD_ONLY
)

_userver_module_find_include(NAMES ev.h libev/ev.h)

_userver_module_find_library(NAMES ev)

_userver_module_end()

if(NOT TARGET libev::libev)
    if(TARGET libev)
        add_library(libev::libev ALIAS libev)
    elseif(libev_ADDED)
        # nghttp2 doesn't use find_package(), but calls find_path() and find_library()
        # so we have to provide libev.a at the _configure_ time, not at build time, =(
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${libev_SOURCE_DIR} ${libev_BINARY_DIR}
        )
        execute_process(
            COMMAND ./configure
            WORKING_DIRECTORY ${libev_BINARY_DIR}
        )
        execute_process(
            COMMAND make
            WORKING_DIRECTORY ${libev_BINARY_DIR}
        )
        execute_process(
            COMMAND rm -rf ${libev_BINARY_DIR}/.libs/libev.so
        )

        add_library(libev STATIC IMPORTED)
        target_include_directories(libev INTERFACE ${libev_BINARY_DIR})
        set_target_properties(libev PROPERTIES IMPORTED_LOCATION ${libev_BINARY_DIR}/.libs/libev.a)

        # For nghttp2 installed from CPM
        list(APPEND CMAKE_INCLUDE_PATH ${libev_BINARY_DIR})
        list(APPEND CMAKE_LIBRARY_PATH ${libev_BINARY_DIR}/.libs)

        add_library(libev::libev ALIAS libev)
    else()
        message(FATAL_ERROR "libev cmake target not found, don't know how to link")
    endif()
endif()
