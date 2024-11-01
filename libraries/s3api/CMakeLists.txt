project(userver-lib-s3api CXX)

find_package(Pugixml REQUIRED)

userver_module(lib-s3api
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE Pugixml
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

_userver_directory_install(COMPONENT lib-s3api FILES
    "${USERVER_ROOT_DIR}/cmake/modules/FindPugixml.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

if(USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()
