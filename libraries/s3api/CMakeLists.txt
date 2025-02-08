project(userver-s3api CXX)

find_package(pugixml REQUIRED)

userver_module(
    s3api
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE pugixml::pugixml
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

_userver_directory_install(
    COMPONENT s3api
    FILES "${USERVER_ROOT_DIR}/cmake/modules/Findpugixml.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if(USERVER_FEATURE_UTEST)
    add_subdirectory(utest)
endif()
