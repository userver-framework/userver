include_guard(GLOBAL)

set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

file(GLOB _userver_install_support_modules CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/install-support/*.cmake")
foreach(_module IN LISTS _userver_install_support_modules)
    include("${_module}")
endforeach()
unset(_userver_install_support_modules)
unset(_module)

# Must run at include time, after all install-support functions are defined.
# Detects the OS codename, sets DEPENDENCIES_FILESTEM, and removes stale
# cpack*.inc fragments from a previous configure run.
_userver_prepare_components()
