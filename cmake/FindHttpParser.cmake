find_path(HTTP_PARSER_INCLUDE_DIR http_parser.h)
find_library(HTTP_PARSER_LIBRARIES NAMES http_parser)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  HttpParser
  "Cannot find development files for the HTTP Parser library. Please install libhttp-parser-dev."
  HTTP_PARSER_LIBRARIES HTTP_PARSER_INCLUDE_DIR)
mark_as_advanced(HTTP_PARSER_LIBRARIES HTTP_PARSER_INCLUDE_DIR)
