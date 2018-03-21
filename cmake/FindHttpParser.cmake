find_path(HTTP_PARSER_INCLUDE_DIR http_parser.h)
find_library(HTTP_PARSER_LIBRARIES NAMES http_parser)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  http_parser DEFAULT_MSG HTTP_PARSER_LIBRARIES HTTP_PARSER_INCLUDE_DIR)
mark_as_advanced(HTTP_PARSER_LIBRARIES HTTP_PARSER_INCLUDE_DIR)

if(NOT HTTP_PARSER_FOUND)
  message(FATAL_ERROR "Cannot find development files for the HttpParser library. Please try installing libhttp-parser-dev.")
endif()
