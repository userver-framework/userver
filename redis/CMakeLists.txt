project(yandex-taxi-userver-redis)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
)

# Hiredis' config doesn't set _FOUND :(
find_package(Hiredis 0.13.3)
append_debian_depends("libhiredis-dev")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    yandex-taxi-userver-core
    yandex-taxi-backend-common-cpp-redis
    ${HIREDIS_LIBRARIES}
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:yandex-taxi-userver-core,INCLUDE_DIRECTORIES>
)

add_subdirectory(../backend-common-cpp/redis ${CMAKE_CURRENT_BINARY_DIR}/backend-common-cpp-redis/)
target_include_directories(
  yandex-taxi-backend-common-cpp-redis
  PUBLIC
    ../core/include/
  PRIVATE
    ../core/src/
    ${SPDLOG_INCLUDE_DIRS}
)


add_subdirectory(tools/redisclient)
