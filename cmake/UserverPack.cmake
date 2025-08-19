set(CPACK_PACKAGE_VENDOR "userver.tech")
set(CPACK_PACKAGE_DESCRIPTION
    "🐙 userver framework\
    A modern open source asynchronous framework with a rich set of \
    abstractions for fast and comfortable creation of C++ microservices, \
    services and utilities."
)

option(USERVER_INSTALL_MULTIPACKAGE "Whether create per-component packages" OFF)
if(USERVER_INSTALL_MULTIPACKAGE)
    set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
else()
    set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
endif()

set(CPACK_PACKAGE_NAME "libuserver-all-dev")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGE_VERSION "${USERVER_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${USERVER_MAJOR_VERSION}")
set(CPACK_PACKAGE_VERSION_MINOR "${USERVER_MINOR_VERSION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://userver.tech")
set(CPACK_PACKAGE_CONTACT "Antony Polukhin <antoshkka@userver.tech>")
set(CPACK_RESOURCE_FILE_LICENSE "${USERVER_ROOT_DIR}/LICENSE")

# The installation path directory should have 0755 permissions:
set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE
)

if(DEPENDENCIES_FILESTEM)
    execute_process(
        COMMAND cat "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}.md"
        COMMAND tr "\n" " "
        COMMAND sed "s/ \\(.\\)/, \\1/g"
        OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_DEPENDS
    )
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
endif()

if(CPACK_COMPONENTS_GROUPING STREQUAL ONE_PER_GROUP)
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS ON)
endif()

# Must go just before CPack
include("${CMAKE_BINARY_DIR}/cpack.variables.inc")

# CPack setup is ready. Including it:
include(CPack)

# Must go after all modules
include("${CMAKE_BINARY_DIR}/cpack.inc")
