_userver_module_begin(
    NAME libnghttp2
    DEBIAN_NAMES libnghttp2-dev
    FORMULA_NAMES nghttp2
    PACMAN_NAMES libnghttp2
    CPM_NAME libnghttp2
    CPM_VERSION 1.66.0
    CPM_URL https://github.com/nghttp2/nghttp2/archive/v1.66.0.tar.gz
    CPM_URL_HASH SHA256=e2505d08aafac303ed8e3b94bf8caafa55ddcc198f89b254bb79a61a379498ea
    CPM_OPTIONS "BUILD_STATIC_LIBS ON" "BUILD_SHARED_LIBS OFF" "ENABLE_APP OFF" "ENABLE_EXAMPLES OFF" "ENABLE_DOC OFF"
)

_userver_module_find_include(NAMES nghttp2/nghttp2.h)

_userver_module_find_library(NAMES nghttp2)

_userver_module_end()

if(NOT TARGET libnghttp2::nghttp2)
    if(TARGET libnghttp2)
        add_library(libnghttp2::nghttp2 ALIAS libnghttp2)
    elseif(TARGET nghttp2_static)
        add_library(libnghttp2::nghttp2 ALIAS nghttp2_static)
    else()
        message(FATAL_ERROR "libnghttp2{,_static} cmake target not found, don't know how to link")
    endif()
endif()

if(CPM_PACKAGE_libnghttp2_SOURCE_DIR)
    # CPM-downloaded CURL does not find CPM-downloaded libnghttp2 automatically.
    set(nghttp2_combined_include_dir ${CPM_PACKAGE_libnghttp2_BINARY_DIR}/combined_includes)
    add_custom_target(
        userver-libnghttp2-curl-bridge
        COMMAND ${CMAKE_COMMAND} -E make_directory "${nghttp2_combined_include_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CPM_PACKAGE_libnghttp2_SOURCE_DIR}/lib/includes"
                "${nghttp2_combined_include_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CPM_PACKAGE_libnghttp2_BINARY_DIR}/lib/includes"
                "${nghttp2_combined_include_dir}"
        COMMENT "Copying libnghttp2 includes to a directory consumable by CURL"
        VERBATIM
    )
    get_target_property(nghttp2_original_target libnghttp2::nghttp2 ALIASED_TARGET)
    add_dependencies(${nghttp2_original_target} userver-libnghttp2-curl-bridge)
    set(NGHTTP2_INCLUDE_DIR
        "${nghttp2_combined_include_dir}"
        CACHE INTERNAL "" FORCE
    )
    set(NGHTTP2_LIBRARY
        "$<TARGET_FILE:libnghttp2::nghttp2>"
        CACHE INTERNAL "" FORCE
    )
endif()
