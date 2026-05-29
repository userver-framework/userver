include_guard(GLOBAL)

if(userver_scylla_FOUND)
    return()
endif()

find_package(userver REQUIRED COMPONENTS core)

include("${USERVER_CMAKE_DIR}/SetupScyllaDeps.cmake")

set(userver_scylla_FOUND TRUE)