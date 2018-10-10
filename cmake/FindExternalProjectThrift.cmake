include(ExternalProject)

ExternalProject_Add(
	thrift
        GIT_REPOSITORY git@github.yandex-team.ru:taxi-external/thrift.git
        GIT_TAG 0.11.0
        TIMEOUT 10
        CMAKE_ARGS -DBUILD_TESTING=OFF
                -DBUILD_EXAMPLES=OFF
                -DBUILD_TUTORIALS=OFF
                -DWITH_JAVA=OFF
                -DWITH_PYTHON=OFF
                -DWITH_HASKELL=OFF
                -DWITH_SHARED_LIB=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE OFF
        LOG_BUILD OFF)

ExternalProject_Get_Property(thrift source_dir binary_dir)

# INTERFACE_INCLUDE_DIRECTORIES will be created at build step,
# but Cmake doesn't support non-existing directories for INTERFACE_INCLUDE_DIRECTORIES
set(THRIFT_BINARY_INCLUDE_DIR "${binary_dir}/")
set(THRIFT_INCLUDE_DIR "${source_dir}/lib/cpp/src")
file(MAKE_DIRECTORY "${THRIFT_BINARY_INCLUDE_DIR}")
file(MAKE_DIRECTORY "${THRIFT_INCLUDE_DIR}")


add_library(libthrift IMPORTED STATIC GLOBAL)
add_dependencies(libthrift thrift)
set_target_properties(libthrift PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/thrift.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${THRIFT_BINARY_INCLUDE_DIR};${THRIFT_INCLUDE_DIR}"
        )

