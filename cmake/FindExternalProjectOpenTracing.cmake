include(ExternalProject)

ExternalProject_Add(
        opentracing
        GIT_REPOSITORY https://github.yandex-team.ru/taxi-external/opentracing-cpp.git
        GIT_TAG v1.5.0
        TIMEOUT 10
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}
        CMAKE_ARGS -DBUILD_SHARED_LIBS=OFF
                -DBUILD_DYNAMIC_LOADING=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        # Disable install step
        INSTALL_COMMAND ""
        # Disable update command, since we use predefined stable version
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE OFF
        LOG_BUILD OFF)

ExternalProject_Get_Property(opentracing source_dir binary_dir)

# INTERFACE_INCLUDE_DIRECTORIES will be created at build step,
# but Cmake doesn't support non-existing directories for INTERFACE_INCLUDE_DIRECTORIES
set(OPENTRACING_BINARY_INCLUDE_DIR "${binary_dir}/include")
set(OPENTRACING_INCLUDE_DIR "${source_dir}/include")
set(OPENTRACING_3RD_PARTY_INCLUDE_DIR "${source_dir}/3rd_party/include")
file(MAKE_DIRECTORY "${OPENTRACING_BINARY_INCLUDE_DIR}")
file(MAKE_DIRECTORY "${OPENTRACING_INCLUDE_DIR}")
file(MAKE_DIRECTORY "${OPENTRACING_3RD_PARTY_INCLUDE_DIR}")


add_library(libopentracing IMPORTED STATIC GLOBAL)
add_dependencies(libopentracing opentracing)
set_target_properties(libopentracing PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/output/libopentracing.a"
	"IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
	"INTERFACE_INCLUDE_DIRECTORIES" "${OPENTRACING_BINARY_INCLUDE_DIR};${OPENTRACING_3RD_PARTY_INCLUDE_DIR};${OPENTRACING_INCLUDE_DIR}"
        )
