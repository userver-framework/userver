include(ExternalProject)

if (TARGET benchmark)
    return()
endif()

ExternalProject_Add(
        gbench
        SOURCE_DIR ${userver_SOURCE_DIR}/submodules/google-benchmark
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
                -DBENCHMARK_ENABLE_TESTING=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
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

ExternalProject_Get_Property(gbench source_dir binary_dir)

set (GBENCH_INCLUDE_DIRS ${source_dir}/include)
file (MAKE_DIRECTORY ${GBENCH_INCLUDE_DIRS})

add_library(libbenchmark IMPORTED STATIC GLOBAL)
add_dependencies(libbenchmark gbench)
set_target_properties(libbenchmark PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/src/libbenchmark.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${GTEST_INCLUDE_DIRS}"
        )

add_library(libbenchmark_main IMPORTED STATIC GLOBAL)
add_dependencies(libbenchmark_main libbenchmark)
set_target_properties(libbenchmark_main PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/src/libbenchmark_main.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${GTEST_INCLUDE_DIRS}"
        )

set (GBENCH_LIBRARY libbenchmark)
set (GBENCH_MAIN_LIBRARY libbenchmark_main)
set (GBENCH_LIBRARIES ${GBENCH_LIBRARY})
set (GBENCH_BOTH_LIBRARIES ${GBENCH_LIBRARY} ${GBENCH_MAIN_LIBRARY})
