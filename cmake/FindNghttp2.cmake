find_path(NGHTTP2_INCLUDE_DIR nghttp2/nghttp2.h)
find_library(NGHTTP2_LIBRARIES NAMES nghttp2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Nghttp2
  "Cannot find development files for the nghttp2 library. Please install libnghttp2-dev package."
  NGHTTP2_LIBRARIES NGHTTP2_INCLUDE_DIR)
mark_as_advanced(NGHTTP2_LIBRARIES NGHTTP2_INCLUDE_DIR)

