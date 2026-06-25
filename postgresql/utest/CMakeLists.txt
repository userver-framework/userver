project(userver-postgresql-utest)

file(GLOB_RECURSE POSTGRESQL_LIB_UTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_library(${PROJECT_NAME} STATIC ${POSTGRESQL_LIB_UTEST_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC userver::postgresql userver::utest)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

_userver_directory_install(
    COMPONENT postgresql
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
)
_userver_install_targets(COMPONENT postgresql TARGETS ${PROJECT_NAME})
