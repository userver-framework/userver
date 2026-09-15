# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK "Download and setup ydb-cpp-sdk" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_YDBCPPSDK_VERSION
    3.22.0
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

# GitHub archive SHA256 for
# https://github.com/ydb-platform/ydb-cpp-sdk/archive/v<version>.tar.gz
function(_userver_ydb_cpp_sdk_archive_sha256 version out_var)
    if(version STREQUAL "3.21.0")
        set(_hash 3a7b4e019753cece81d74db01a1a0fa9d317e60b79c7f19aeb4b5776af1cc33c)
    elseif(version STREQUAL "3.21.1")
        set(_hash 13cedb6e8f730f5bf45e35b0b40c5d0ebee0f07e51ac7cf09f72495a2d389de2)
    elseif(version STREQUAL "3.22.0")
        set(_hash 1c2ccb5bb42139532eb6fe931c9da097f532afc12411b7b3214cdd50d7c10ba0)
    else()
        message(
            FATAL_ERROR
            "Unknown ydb-cpp-sdk version '${version}'. "
            "Add its archive SHA256 to _userver_ydb_cpp_sdk_archive_sha256() in "
            "cmake/SetupYdbCppSDK.cmake. Known versions: 3.21.0, 3.21.1, 3.22.0."
        )
    endif()
    set(${out_var} "${_hash}" PARENT_SCOPE)
endfunction()

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
    URL https://github.com/aklomp/base64/archive/v0.5.2.tar.gz
    URL_HASH SHA256=723a0f9f4cf44cf79e97bcc315ec8f85e52eb104c8882942c3f2fba95acc080d
    OPTIONS "CMAKE_SKIP_INSTALL_RULES ON"
)
write_package_stub(base64)
add_library(aklomp::base64 ALIAS base64)

cpmaddpackage(
    NAME jwt-cpp
    VERSION 0.7.0
    URL https://github.com/Thalhammer/jwt-cpp/archive/v0.7.0.tar.gz
    URL_HASH SHA256=b9eb270e3ba8221e4b2bc38723c9a1cb4fa6c241a42908b9a334daff31137406
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

_userver_ydb_cpp_sdk_archive_sha256("${USERVER_YDBCPPSDK_VERSION}" _userver_ydb_cpp_sdk_url_hash)

cpmaddpackage(
    NAME ydb-cpp-sdk
    VERSION ${USERVER_YDBCPPSDK_VERSION}
    URL https://github.com/ydb-platform/ydb-cpp-sdk/archive/v${USERVER_YDBCPPSDK_VERSION}.tar.gz
    URL_HASH SHA256=${_userver_ydb_cpp_sdk_url_hash}
    OPTIONS ${_userver_ydb_cpp_sdk_options}
)

set(CMAKE_MODULE_PATH "${_userver_ydb_saved_cmake_module_path}")

list(APPEND ydb-cpp-sdk_INCLUDE_DIRS ${ydb-cpp-sdk_SOURCE_DIR} ${ydb-cpp-sdk_SOURCE_DIR}/include
     ${ydb-cpp-sdk_BINARY_DIR}
)
mark_targets_as_system("${ydb-cpp-sdk_SOURCE_DIR}")
