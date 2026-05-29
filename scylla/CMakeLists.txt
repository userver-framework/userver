project(userver-scylla CXX)

include(SetupScyllaDeps)

userver_module(
        scylla
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        LINK_LIBRARIES scylla_cpp_driver
        DEPENDS core
        EMBED_FILES src/storages/scylla/component.yaml
)

_userver_directory_install(
        COMPONENT scylla
        FILES "${USERVER_ROOT_DIR}/cmake/SetupScyllaDeps.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver
)
