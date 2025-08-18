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

# DEB dependencies:
execute_process(COMMAND lsb_release -cs OUTPUT_VARIABLE OS_CODENAME)
if(OS_CODENAME MATCHES "^bookworm")
    set(DEPENDENCIES_FILESTEM "debian-12")
elseif(OS_CODENAME MATCHES "^bullseye")
    set(DEPENDENCIES_FILESTEM "debian-11")
elseif(OS_CODENAME MATCHES "^noble")
    set(DEPENDENCIES_FILESTEM "ubuntu-24.04")
elseif(OS_CODENAME MATCHES "^jammy")
    set(DEPENDENCIES_FILESTEM "ubuntu-22.04")
elseif(OS_CODENAME MATCHES "^impish")
    set(DEPENDENCIES_FILESTEM "ubuntu-21.04")
elseif(OS_CODENAME MATCHES "^focal")
    set(DEPENDENCIES_FILESTEM "ubuntu-20.04")
elseif(OS_CODENAME MATCHES "^bionic")
    set(DEPENDENCIES_FILESTEM "ubuntu-18.04")
endif()

if(DEPENDENCIES_FILESTEM)
    file(GLOB DEPENDENCIES_FILES
         RELATIVE "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}"
         "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}/*"
    )
    foreach(MODULE ${DEPENDENCIES_FILES})
        string(TOUPPER "${MODULE}" MODULE_UPPER)
        execute_process(
            COMMAND cat "${USERVER_ROOT_DIR}/scripts/docs/en/deps/${DEPENDENCIES_FILESTEM}/${MODULE}"
            COMMAND tr "\n" " "
            COMMAND sed "s/ \\(.\\)/, \\1/g"
            OUTPUT_VARIABLE CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_DEPENDS
        )
	set(CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_NAME libuserver-${MODULE}-dev)
	set(CPACK_DEBIAN_${MODULE_UPPER}_PACKAGE_CONFLICTS libuserver-all-dev)
    endforeach()

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

# CPack setup is ready. Including it:
include(CPack)
