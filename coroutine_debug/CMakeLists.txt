project(userver-coroutine-debug CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

find_package(libunwind REQUIRED)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
  PRIVATE
    libunwind
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} PRIVATE
  $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)
