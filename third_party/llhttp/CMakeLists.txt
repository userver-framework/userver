project(userver-llhttp VERSION 9.2.0)

set(CMAKE_C_STANDARD 99)

# Source code
set(LLHTTP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/llhttp.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/http.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/api.c
)

set(LLHTTP_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/include/llhttp.h
)

add_library(${PROJECT_NAME} OBJECT ${LLHTTP_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
_userver_install_targets(COMPONENT universal TARGETS ${PROJECT_NAME})
_userver_directory_install(COMPONENT universal
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include 
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..
)

