include(ExternalProject)

if (TARGET gtest)
    return()
endif()

ExternalProject_Add(
        gtest
        GIT_REPOSITORY https://github.yandex-team.ru/taxi-external/googletest.git
        GIT_TAG release-1.8.0
        TIMEOUT 10
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS -DBUILD_GTEST:BOOL=ON -DBUILD_GMOCK:BOOL=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        # CMAKE_ARGS ${ext_CMAKE_ARGS}
        # Disable install step
        INSTALL_COMMAND ""
        # Disable update command, since we use predefined stable version
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)

add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

ExternalProject_Get_Property(gtest source_dir binary_dir)

set (GTEST_INCLUDE_DIRS ${source_dir}/googletest/include)
set (GMOCK_INCLUDE_DIRS ${source_dir}/googlemock/include)
file (MAKE_DIRECTORY ${GTEST_INCLUDE_DIRS})
file (MAKE_DIRECTORY ${GMOCK_INCLUDE_DIRS})


set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googletest/libgtest.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${GTEST_INCLUDE_DIRS}"
        )

set( GTEST_LIBRARIES libgtest )
set( GTEST_LIBRARY ${GTEST_LIBRARIES})
set( GTEST_MAIN_LIBRARY ${binary_dir}/googletest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a )

# Add gmock
ExternalProject_Add(
        gmock
        GIT_REPOSITORY https://github.yandex-team.ru/taxi-external/googletest.git
        GIT_TAG release-1.8.0
        TIMEOUT 10
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS -DBUILD_GTEST:BOOL=OFF -DBUILD_GMOCK:BOOL=ON
        #CMAKE_ARGS ${ext_CMAKE_ARGS}
        # Disable install step
        INSTALL_COMMAND ""
        # Disable update command, since we use predefined stable version
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)

add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gmock)

ExternalProject_Get_Property(gmock source_dir binary_dir)
set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${GMOCK_INCLUDE_DIRS}"
        )
#ExternalProject_Get_Property(googlemock source_dir)

ExternalProject_Get_Property(gmock binary_dir)
set(GMOCK_LIBRARIES libgmock)

add_dependencies(gmock gtest)


# GTEST_BOTH_LIBRARIES
set (GTEST_BOTH_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
