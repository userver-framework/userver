project(userver-s3api-utest)

file(GLOB_RECURSE S3API_LIB_UTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_library(${PROJECT_NAME} STATIC ${S3API_LIB_UTEST_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC userver::s3api userver::utest)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

_userver_directory_install(
    COMPONENT s3api
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
)
_userver_install_targets(COMPONENT s3api TARGETS ${PROJECT_NAME})
