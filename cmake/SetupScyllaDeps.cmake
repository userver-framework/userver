include_guard(GLOBAL)

find_library(SCYLLA_CPP_DRIVER_LIB scylla-cpp-driver)
find_path(SCYLLA_CPP_DRIVER_INCLUDE cassandra.h)

if(NOT SCYLLA_CPP_DRIVER_LIB OR NOT SCYLLA_CPP_DRIVER_INCLUDE)
    message(FATAL_ERROR "ScyllaDB cpp-driver not found")
endif()

message(STATUS "ScyllaDB cpp-driver: ${SCYLLA_CPP_DRIVER_LIB}")

add_library(scylla_cpp_driver SHARED IMPORTED)
set_target_properties(scylla_cpp_driver PROPERTIES
    IMPORTED_LOCATION "${SCYLLA_CPP_DRIVER_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${SCYLLA_CPP_DRIVER_INCLUDE}"
)