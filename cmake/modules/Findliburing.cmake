_userver_module_begin(
    NAME liburing
    DEBIAN_NAMES liburing-dev
    RPM_NAMES liburing-devel
    PACMAN_NAMES liburing
    CPM_NAME liburing
    CPM_VERSION 2.9
    CPM_URL https://github.com/axboe/liburing/archive/liburing-2.9.tar.gz
    CPM_URL_HASH SHA256=897b1153b55543e8b92a5a3eb9b906537a5fedcf8afaf241f8b8787940c79f8d
    CPM_DOWNLOAD_ONLY
)

_userver_module_find_include(NAMES liburing.h)

_userver_module_find_library(NAMES uring)

_userver_module_end()

function(_userver_liburing_execute_process)
    execute_process(${ARGV} RESULT_VARIABLE RET)
    if(NOT ("${RET}" EQUAL 0))
        message(FATAL_ERROR "Command failed with return code ${RET} (${ARGV})")
    endif()
endfunction()

if(NOT TARGET liburing)
    # CPM sets *_ADDED=YES only on the first CPMAddPackage in a configure; later
    # find_package(liburing) (e.g. from rocks/) still has SOURCE_DIR but ADDED=NO.
    if(liburing_SOURCE_DIR)
        # liburing ships with configure+Make, not CMake.
        if(NOT EXISTS ${liburing_BINARY_DIR}/.built)
            _userver_liburing_execute_process(
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${liburing_SOURCE_DIR} ${liburing_BINARY_DIR}
            )
            _userver_liburing_execute_process(COMMAND ./configure --use-libc WORKING_DIRECTORY ${liburing_BINARY_DIR})
            _userver_liburing_execute_process(COMMAND make -C src WORKING_DIRECTORY ${liburing_BINARY_DIR})
            file(GLOB _userver_liburing_shared_libs "${liburing_BINARY_DIR}/src/liburing.so*")
            if(_userver_liburing_shared_libs)
                _userver_liburing_execute_process(COMMAND rm -f ${_userver_liburing_shared_libs})
            endif()
            unset(_userver_liburing_shared_libs)
            _userver_liburing_execute_process(COMMAND ${CMAKE_COMMAND} -E touch ${liburing_BINARY_DIR}/.built)
        endif()

        add_library(liburing STATIC IMPORTED GLOBAL)
        target_include_directories(liburing INTERFACE ${liburing_BINARY_DIR}/src/include)
        set_target_properties(liburing PROPERTIES IMPORTED_LOCATION ${liburing_BINARY_DIR}/src/liburing.a)
    else()
        message(FATAL_ERROR "liburing cmake target not found, don't know how to link")
    endif()
endif()

# userver uses liburing::uring; RocksDB's exported cmake expects uring::uring.
if(NOT TARGET liburing::uring)
    add_library(liburing::uring ALIAS liburing)
endif()
if(NOT TARGET uring::uring)
    add_library(uring::uring ALIAS liburing)
endif()
