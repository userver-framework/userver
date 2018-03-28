find_path(JEMALLOC_INCLUDE_DIR jemalloc/jemalloc.h)
find_library(JEMALLOC_LIBRARIES NAMES jemalloc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Jemalloc
  "Cannot find development files for the jemalloc library. Please install libyandex-taxi-jemalloc-dev."
  JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR)
mark_as_advanced(JEMALLOC_LIBRARIES JEMALLOC_INCLUDE_DIR)
