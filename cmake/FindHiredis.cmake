find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARIES NAMES hiredis)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  hiredis DEFAULT_MSG HIREDIS_LIBRARIES HIREDIS_INCLUDE_DIR)
mark_as_advanced(HIREDIS_LIBRARIES HIREDIS_INCLUDE_DIR)

if(NOT HIREDIS_FOUND)
	message(FATAL_ERROR "Cannot find development files for the hiredis library. Please install hiredis-dev.")
endif()
