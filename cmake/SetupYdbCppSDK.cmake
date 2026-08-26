# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK "Download and setup ydb-cpp-sdk" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_YDBCPPSDK_VERSION
    3.21.1
    CACHE STRING "ydb-cpp-sdk version"
)
set(USERVER_YDBCPPSDK_COMPONENTS
    Coordination
    Driver
    FederatedTopic
    Iam
    Operation
    Query
    Result
    Scheme
    SolomonStats
    Table
    Topic
    Types
)

set(USERVER_YDBCPPSDK_DEB_PREFIX /usr/share/yandex)
if(EXISTS "${USERVER_YDBCPPSDK_DEB_PREFIX}/lib/cmake/ydb-cpp-sdk")
    list(PREPEND CMAKE_PREFIX_PATH "${USERVER_YDBCPPSDK_DEB_PREFIX}")
endif()

if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK)
        find_package(ydb-cpp-sdk ${USERVER_YDBCPPSDK_VERSION} QUIET CONFIG COMPONENTS ${USERVER_YDBCPPSDK_COMPONENTS})
    else()
        find_package(
            ydb-cpp-sdk ${USERVER_YDBCPPSDK_VERSION} REQUIRED CONFIG COMPONENTS ${USERVER_YDBCPPSDK_COMPONENTS}
        )
    endif()

    if(ydb-cpp-sdk_FOUND)
        return()
    endif()
endif()

include(DownloadUsingCPM)
include(SetupBrotli)
include(SetupAbseil)

# ydb-cpp-sdk CPMAddPackage's Abseil as "abseil-cpp". If Abseil came from the
# system (SetupAbseil returned early), register that CPM name so ydb does not
# download a second copy that clashes on absl_* targets.
if(NOT "abseil-cpp" IN_LIST CPM_PACKAGES AND (TARGET absl::base OR absl_FOUND))
    cpmregisterpackage(abseil-cpp "${USERVER_ABSEIL_VERSION}")
endif()

cpmaddpackage(
    NAME base64
    VERSION 0.5.2
    GITHUB_REPOSITORY aklomp/base64
    GIT_SHALLOW TRUE
    OPTIONS "CMAKE_SKIP_INSTALL_RULES ON"
)
write_package_stub(base64)
add_library(aklomp::base64 ALIAS base64)

cpmaddpackage(
    NAME jwt-cpp
    VERSION 0.7.0
    GITHUB_REPOSITORY Thalhammer/jwt-cpp
    GIT_SHALLOW TRUE
    OPTIONS "JWT_BUILD_EXAMPLES OFF" "CMAKE_SKIP_INSTALL_RULES ON"
)
write_package_stub(jwt-cpp)

set(RAPIDJSON_INCLUDE_DIRS "${USERVER_THIRD_PARTY_DIRS}/rapidjson/include")

if(TARGET userver-api-common-protos)
    set(YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET userver-api-common-protos)
else()
    include(SetupGoogleProtoApis)
    set(YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET ${api-common-proto_LIBRARY})
endif()

set(_userver_ydb_cpp_sdk_options
    "Brotli_VERSION ${Brotli_VERSION}" "RAPIDJSON_INCLUDE_DIRS ${RAPIDJSON_INCLUDE_DIRS}"
    "YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET ${YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET}" "YDB_SDK_DEPENDENCY_MODE SYSTEM"
    "YDB_SDK_EXAMPLES OFF"
)
set(_userver_ydb_google_proto_include_dir "${USERVER_GOOGLE_COMMON_PROTOS}")
if(NOT _userver_ydb_google_proto_include_dir)
    set(_userver_ydb_google_proto_include_dir "${api-common-protos_SOURCE_DIR}")
endif()
if(NOT _userver_ydb_google_proto_include_dir)
    set(_userver_ydb_google_proto_include_dir "${CPM_PACKAGE_api-common-protos_SOURCE_DIR}")
endif()
if(_userver_ydb_google_proto_include_dir)
    list(APPEND _userver_ydb_cpp_sdk_options
         "YDB_SDK_GOOGLE_PROTO_INCLUDE_DIR ${_userver_ydb_google_proto_include_dir}"
    )
endif()

set(_userver_ydb_grpc_version "${gRPC_VERSION}")
if(NOT _userver_ydb_grpc_version
   AND USERVER_FEATURE_GRPC
   AND EXISTS "${USERVER_ROOT_DIR}/grpc/CMakeLists.txt"
)
    # The grpc directory is configured before ydb, but its gRPC_VERSION is directory-scoped.
    get_directory_property(_userver_ydb_grpc_version DIRECTORY "${USERVER_ROOT_DIR}/grpc" DEFINITION gRPC_VERSION)
endif()

set(_userver_ydb_saved_cmake_module_path "${CMAKE_MODULE_PATH}")
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/ydb")

cpmaddpackage(
    NAME ydb-cpp-sdk
    GIT_TAG v${USERVER_YDBCPPSDK_VERSION}
    GITHUB_REPOSITORY ydb-platform/ydb-cpp-sdk
    GIT_SHALLOW TRUE
    OPTIONS ${_userver_ydb_cpp_sdk_options}
)

set(CMAKE_MODULE_PATH "${_userver_ydb_saved_cmake_module_path}")

list(APPEND ydb-cpp-sdk_INCLUDE_DIRS ${ydb-cpp-sdk_SOURCE_DIR} ${ydb-cpp-sdk_SOURCE_DIR}/include
     ${ydb-cpp-sdk_BINARY_DIR}
)
mark_targets_as_system("${ydb-cpp-sdk_SOURCE_DIR}")
