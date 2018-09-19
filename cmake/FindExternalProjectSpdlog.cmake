include(ExternalProject)

if (NOT TARGET spdlog)

ExternalProject_Add(
        spdlog
        GIT_REPOSITORY https://github.yandex-team.ru/taxi-external/spdlog.git
        GIT_TAG v1.1.0
        TIMEOUT 10
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS -DSPDLOG_BUILD_EXAMPLES=OFF -DSPDLOG_BUILD_TESTING=OFF -DSPDLOG_BUILD_BENCH=OFF
        PATCH_COMMAND pwd
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

