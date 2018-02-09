cmake_minimum_required(VERSION 3.5)

set(PROJECT yandex-taxi-userver)
project(${PROJECT} VERSION 0.0.0) # FIXME

file(GLOB_RECURSE SOURCES *.cpp *.hpp)

find_package(JsonCpp REQUIRED)
find_package(LibEv REQUIRED)

add_library(${PROJECT} STATIC ${SOURCES})
set_target_properties(${PROJECT} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT} PRIVATE
  ${JSONCPP_LIBRARIES}
  ${LIBEV_LIBRARIES}
)

target_include_directories(${PROJECT} PUBLIC ${USERVER_PUBLIC_INCLUDE_DIR})
target_include_directories(${PROJECT} PRIVATE
  .
  ${USERVER_THIRD_PARTY_DIRS}
  ${JSONCPP_INCLUDE_DIR}
  ${LIBEV_INCLUDE_DIR}
)
