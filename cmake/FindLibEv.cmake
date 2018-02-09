find_path(LIBEV_INCLUDE_DIR ev.h)
find_library(LIBEV_LIBRARIES NAMES ev)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libev DEFAULT_MSG LIBEV_LIBRARIES LIBEV_INCLUDE_DIR)
mark_as_advanced(LIBEV_LIBRARIES LIBEV_INCLUDE_DIR)

if(NOT LIBEV_FOUND)
	message(FATAL_ERROR "Cannot find development files for the libev library. Please install libev-dev.")
endif()
