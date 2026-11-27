project(userver-s3api CXX)

find_package(pugixml REQUIRED)

userver_module(
    s3api
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE pugixml::pugixml
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    COMPONENT_DEPENDS core
)

_userver_directory_install(
    COMPONENT s3api
    FILES "${USERVER_ROOT_DIR}/cmake/modules/Findpugixml.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if(USERVER_FEATURE_UTEST)
    add_subdirectory(utest)
endif()

add_subdirectory(tools/cli EXCLUDE_FROM_ALL)

if(USERVER_BUILD_TESTS)
    add_google_tests(userver-s3-cli-unittest)
endif()
