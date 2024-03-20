project(userver-http-parser VERSION 2.9.4)

set(CMAKE_C_STANDARD 99)

# Source code
set(HTTP_PARSER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/http_parser.c
)

set(HTTP_PARSER_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/http_parser.h
)

add_library(${PROJECT_NAME} OBJECT ${HTTP_PARSER_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
_userver_install_targets(COMPONENT universal TARGETS ${PROJECT_NAME})
_userver_directory_install(COMPONENT universal
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)
