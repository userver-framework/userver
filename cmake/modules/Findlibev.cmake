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
        # libev is autoconf'ed, so create targets manually
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${libev_SOURCE_DIR} ${libev_BINARY_DIR}
        )
        execute_process(
            COMMAND ./configure
            WORKING_DIRECTORY ${libev_BINARY_DIR}
        )

        add_custom_command(
            OUTPUT ${libev_BINARY_DIR}/libev.a
            COMMAND make
            WORKING_DIRECTORY ${libev_BINARY_DIR}
        )
        add_library(libev STATIC IMPORTED)
        target_include_directories(libev INTERFACE ${libev_BINARY_DIR})
        set_target_properties(libev PROPERTIES IMPORTED_LOCATION ${libev_BINARY_DIR}/libev.a)
        add_library(libev::libev ALIAS libev)
    endif()
endif()
