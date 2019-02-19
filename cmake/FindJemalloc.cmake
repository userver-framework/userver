# https://github.com/jemalloc/jemalloc/issues/820
if(SANITIZE)
  message(FATAL_ERROR "jemalloc is not compatible with sanitizers, please skip it for SANITIZE-enabled builds")
endif(SANITIZE)

find_path(JEMALLOC_INCLUDE_DIR jemalloc/jemalloc.h)
find_library(JEMALLOC_LIBRARIES NAMES jemalloc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Jemalloc
  "Cannot find development files for the jemalloc library. Please install libyandex-taxi-jemalloc-dev."
  JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR)

if(Jemalloc_FOUND)
  if(NOT TARGET Jemalloc)
    add_library(Jemalloc INTERFACE IMPORTED)
  endif()
  set_property(TARGET Jemalloc PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${JEMALLOC_INCLUDE_DIR}")
  set_property(TARGET Jemalloc PROPERTY INTERFACE_LINK_LIBRARIES "${JEMALLOC_LIBRARIES}")
endif()
mark_as_advanced(JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR)
