include(ExternalProject)

if (NOT TARGET spdlog)

ExternalProject_Add(
        spdlog
        SOURCE_DIR ${userver_SOURCE_DIR}/submodules/spdlog
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS -DSPDLOG_BUILD_EXAMPLES=OFF
                -DSPDLOG_BUILD_TESTING=OFF
                -DSPDLOG_BUILD_BENCH=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        PATCH_COMMAND pwd
        # Use inplace, not download
        DOWNLOAD_COMMAND ""
        # Disable install step
        INSTALL_COMMAND ""
        # Disable update command, since we use predefined stable version
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)
endif()

ExternalProject_Get_Property(spdlog source_dir binary_dir)

set (SPDLOG_INCLUDE_DIRS ${source_dir}/include)
file (MAKE_DIRECTORY ${SPDLOG_INCLUDE_DIRS})

add_library(spdlog_lib INTERFACE IMPORTED GLOBAL)
add_dependencies(spdlog_lib spdlog)
set_target_properties(spdlog_lib PROPERTIES
        "INTERFACE_INCLUDE_DIRECTORIES" "${SPDLOG_INCLUDE_DIRS}"
)
