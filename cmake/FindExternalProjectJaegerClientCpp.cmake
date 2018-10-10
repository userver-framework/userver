include(ExternalProject)

string(REPLACE ";" "|" CMAKE_PREFIX_PATH_ALT_SEP "${CMAKE_PREFIX_PATH};${CMAKE_CURRENT_SOURCE_DIR}/../third_party/nlohmann_json")

ExternalProject_Add(
        jaeger-client-cpp
        GIT_REPOSITORY git@github.yandex-team.ru:taxi-external/jaeger-client-cpp.git
        GIT_TAG v0.4.2-taxi2
        DEPENDS thrift opentracing
        TIMEOUT 10
        CMAKE_ARGS -DJAEGERTRACING_BUILD_EXAMPLES=OFF
                -DJAEGERTRACING_PLUGIN=OFF
                -DBUILD_SHARED_LIBS=OFF
                -DBUILD_TESTING=OFF
                -DHUNTER_ENABLED=OFF
                -DJAEGERTRACING_WITH_YAML_CPP=OFF
                -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH_ALT_SEP}
                -DTHRIFT_HOME=${thrift_DESTDIR}
        LIST_SEPARATOR |
        # Disable install step
        INSTALL_COMMAND ""
        # Disable update command, since we use predefined stable version
        UPDATE_COMMAND ""
        # Wrap download, configure and build steps in a script to log output
        LOG_DOWNLOAD ON
        LOG_CONFIGURE OFF
        LOG_BUILD ON)

ExternalProject_Get_Property(jaeger-client-cpp source_dir binary_dir)

# INTERFACE_INCLUDE_DIRECTORIES will be created at build step,
# but Cmake doesn't support non-existing directories for INTERFACE_INCLUDE_DIRECTORIES
set(JAEGERTRACING_BINARY_INCLUDE_DIR "${binary_dir}/src")
set(JAEGERTRACING_INCLUDE_DIR "${source_dir}/src")
file(MAKE_DIRECTORY "${JAEGERTRACING_BINARY_INCLUDE_DIR}")
file(MAKE_DIRECTORY "${JAEGERTRACING_INCLUDE_DIR}")


add_library(libjaegertracing IMPORTED STATIC GLOBAL)
add_dependencies(libjaegertracing jaeger-client-cpp)
set_target_properties(libjaegertracing PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/libjaegertracing.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT};libopentracing"
        "INTERFACE_INCLUDE_DIRECTORIES" "${JAEGERTRACING_BINARY_INCLUDE_DIR};${JAEGERTRACING_INCLUDE_DIR};${THRIFT_INCLUDE_DIR}"
)
