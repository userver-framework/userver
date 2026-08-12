project(userver-ydb-utest)

file(GLOB_RECURSE YDB_LIB_UTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_library(${PROJECT_NAME} STATIC ${YDB_LIB_UTEST_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC userver::ydb userver::utest)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

_userver_directory_install(
    COMPONENT ydb
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
)
_userver_install_targets(COMPONENT ydb TARGETS ${PROJECT_NAME})
