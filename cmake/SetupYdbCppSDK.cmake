# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK "Download and setup ydb-cpp-sdk" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_YDBCPPSDK_VERSION 3.21.1)
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

cpmaddpackage(
    NAME ydb-cpp-sdk
    VERSION ${USERVER_YDBCPPSDK_VERSION}
    URL https://github.com/ydb-platform/ydb-cpp-sdk/archive/v${USERVER_YDBCPPSDK_VERSION}.tar.gz
    URL_HASH SHA256=13cedb6e8f730f5bf45e35b0b40c5d0ebee0f07e51ac7cf09f72495a2d389de2
    OPTIONS "Brotli_VERSION ${Brotli_VERSION}" "RAPIDJSON_INCLUDE_DIRS ${RAPIDJSON_INCLUDE_DIRS}"
            "YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET ${YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET}" "YDB_SDK_EXAMPLES OFF"
)

list(APPEND ydb-cpp-sdk_INCLUDE_DIRS ${ydb-cpp-sdk_SOURCE_DIR} ${ydb-cpp-sdk_SOURCE_DIR}/include
     ${ydb-cpp-sdk_BINARY_DIR}
)
mark_targets_as_system("${ydb-cpp-sdk_SOURCE_DIR}")
