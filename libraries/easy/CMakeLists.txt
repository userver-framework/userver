project(userver-easy CXX)

userver_module(
    easy
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES userver::postgresql
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    COMPONENT_DEPENDS core
)

if(USERVER_BUILD_SAMPLES)
    # Imitate an installed userver for the leaf find_package(userver) calls.
    file(WRITE "${CMAKE_BINARY_DIR}/package_stubs/userverConfig.cmake" "")

    add_subdirectory(samples)

    # Install the easy samples as a standalone project under
    # share/userver/samples/easy (a dedicated subdirectory so they do not clutter
    # the root userver samples). The trailing slash installs the directory
    # contents, including samples/CMakeLists.txt which becomes the project root.
    _userver_directory_install(
        COMPONENT examples
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/samples/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/userver/samples/easy"
    )

    # Clean up the fake userver installation above to avoid messing with
    # find_package(userver) in the parent project.
    file(REMOVE "${CMAKE_BINARY_DIR}/package_stubs/userverConfig.cmake")
endif()
