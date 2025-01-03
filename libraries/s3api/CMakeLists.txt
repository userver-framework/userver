project(userver-s3api CXX)

if(USERVER_CONAN)
  find_package(pugixml REQUIRED CONFIG)
  add_library(Pugixml ALIAS pugixml::pugixml)
else()
  find_package(Pugixml REQUIRED)
endif()

userver_module(s3api
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE Pugixml
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

_userver_directory_install(COMPONENT s3api FILES
    "${USERVER_ROOT_DIR}/cmake/modules/FindPugixml.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if(USERVER_FEATURE_UTEST)
  add_subdirectory(utest)
endif()
