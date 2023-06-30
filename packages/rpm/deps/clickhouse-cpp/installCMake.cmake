include(CMakePackageConfigHelpers)

if(NOT DEFINED CLICKHOUSE_CPP_VERSION)
  set(CLICKHOUSE_CPP_VERSION "1.0.0")
endif()

set_verbose(CLICKHOUSE_CPP_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/clickhouse-cpp CACHE STRING
  "Installation directory for cmake files, a relative path that "
  "will be joined with ${CMAKE_INSTALL_PREFIX} or an absolute "
  "path.")

set(version_config ${PROJECT_BINARY_DIR}/clickhouse-cpp-config-version.cmake)
set(project_config ${PROJECT_BINARY_DIR}/clickhouse-cpp-config.cmake)

# Generate the version, config and target files into the build directory.
write_basic_package_version_file(
  ${version_config}
  VERSION ${CLICKHOUSE_CPP_VERSION}
  COMPATIBILITY AnyNewerVersion)

export(TARGETS clickhouse-cpp-lib NAMESPACE clickhouse-cpp::
  FILE ${PROJECT_BINARY_DIR}/clickhouse-cpp-targets.cmake)

# Install version, config and target files.
    
install(
  FILES ${project_config} ${version_config}
  DESTINATION ${CLICKHOUSE_CPP_CMAKE_DIR})

install(EXPORT clickhouse-cpp-lib DESTINATION ${CLICKHOUSE_CPP_CMAKE_DIR}
  NAMESPACE clickhouse-cpp::)

#install(FILES $<TARGET_PDB_FILE:${INSTALL_TARGETS}>
#  DESTINATION ${CLICKHOUSE_CPP_LIB_DIR} OPTIONAL)

#install(FILES "${pkgconfig}" DESTINATION "${CLICKHOUSE_CPP_PKGCONFIG_DIR}")
