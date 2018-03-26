find_path(
  JSONCPP_INCLUDE_DIR json/json.h
  PATHS
    "/usr/include/jsoncpp")
find_library(JSONCPP_LIBRARIES NAMES jsoncpp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  JsonCpp
  "Cannot find development files for the JsonCpp library. Please install libjsoncpp-dev."
  JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)
mark_as_advanced(JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)
