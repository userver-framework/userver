find_path(
  JSONCPP_INCLUDE_DIR json/json.h
  PATHS
    "/usr/include/jsoncpp")

find_library(JSONCPP_LIB jsoncpp)
set(JSONCPP_LIBRARIES ${JSONCPP_LIB})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  JsonCpp DEFAULT_MSG
  JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)
mark_as_advanced(JSONCPP_LIBRARIES JSONCPP_INCLUDE_DIR)

if(NOT JSONCPP_FOUND)
  message(FATAL_ERROR "Cannot find development files for the JsonCpp library. Please try installing libjsoncpp-dev.")
endif()
