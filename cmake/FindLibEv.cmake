find_path(LIBEV_INCLUDE_DIR ev.h)
find_library(LIBEV_LIBRARIES NAMES ev)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	LibEv
	"Cannot find development files for the libev library. Please install libev-dev."
	LIBEV_LIBRARIES LIBEV_INCLUDE_DIR)

if(LibEv_FOUND)
  if(NOT TARGET LibEv)
    add_library(LibEv INTERFACE IMPORTED)
  endif()
  set_property(TARGET LibEv PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LIBEV_INCLUDE_DIR}")
  set_property(TARGET LibEv PROPERTY INTERFACE_LINK_LIBRARIES "${LIBEV_LIBRARIES}")
endif()
mark_as_advanced(LIBEV_LIBRARIES LIBEV_INCLUDE_DIR)
