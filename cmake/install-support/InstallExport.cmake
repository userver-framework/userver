include_guard(GLOBAL)

# Installs the CMake export sets (targets-*.cmake files) for every exported
# component into the build-type-specific subdirectory of the install tree.
#
# Must be called once at the top level after all _userver_install_targets()
# calls (i.e. after all add_subdirectory() calls).
# Depends on _userver_build_type_export_subdir_and_suffix from ModuleHelpers.cmake,
# which must be included before this function is called.
function(_userver_export_targets)
    if(NOT USERVER_INSTALL)
        return()
    endif()

    _userver_build_type_export_subdir_and_suffix(_export_subdir _export_suffix)

    get_property(_exported_components GLOBAL PROPERTY USERVER_EXPORTED_COMPONENTS)

    foreach(_comp IN LISTS _exported_components)
        install(
            EXPORT userver-targets-${_comp}
            FILE userver-targets-${_comp}${_export_suffix}.cmake
            NAMESPACE userver::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/${_export_subdir}/${_comp}
            COMPONENT ${_comp}
        )
    endforeach()
endfunction()
