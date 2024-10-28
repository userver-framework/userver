project(userver-redis-utest)

file(GLOB_RECURSE UTESTLIB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_library(${PROJECT_NAME} STATIC ${UTESTLIB_SOURCES})
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    userver-utest
    userver-redis
)
_userver_directory_install(COMPONENT redis
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
)
_userver_install_targets(COMPONENT redis TARGETS ${PROJECT_NAME})
