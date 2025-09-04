option(USERVER_DOWNLOAD_PACKAGE_YDBCPPSDK "Download and setup ydb-cpp-sdk" ${USERVER_DOWNLOAD_PACKAGES})

include(DownloadUsingCPM)
include(SetupBrotli)

cpmaddpackage(
    NAME
    base64
    VERSION
    0.5.2
    GITHUB_REPOSITORY
    aklomp/base64
    OPTIONS
    "CMAKE_SKIP_INSTALL_RULES ON"
)
write_package_stub(base64)
add_library(aklomp::base64 ALIAS base64)

cpmaddpackage(
    NAME
    jwt-cpp
    VERSION
    0.7.0
    GITHUB_REPOSITORY
    Thalhammer/jwt-cpp
    OPTIONS
    "JWT_BUILD_EXAMPLES OFF"
    "CMAKE_SKIP_INSTALL_RULES ON"
)
write_package_stub(jwt-cpp)

write_package_stub(RapidJSON)
set(RAPIDJSON_INCLUDE_DIRS "${USERVER_THIRD_PARTY_DIRS}/rapidjson/include")

if(TARGET userver-api-common-protos)
    set(YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET userver-api-common-protos)
else()
    include(SetupGoogleProtoApis)
    set(YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET ${api-common-proto_LIBRARY})
endif()

cpmaddpackage(
    NAME
    ydb-cpp-sdk
    GIT_TAG
    v3.5.1
    GITHUB_REPOSITORY
    ydb-platform/ydb-cpp-sdk
    OPTIONS
    "Brotli_VERSION ${Brotli_VERSION}"
    "RAPIDJSON_INCLUDE_DIRS ${RAPIDJSON_INCLUDE_DIRS}"
    "YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET ${YDB_SDK_GOOGLE_COMMON_PROTOS_TARGET}"
    "YDB_SDK_EXAMPLES OFF"
)

list(APPEND ydb-cpp-sdk_INCLUDE_DIRS ${ydb-cpp-sdk_SOURCE_DIR} ${ydb-cpp-sdk_SOURCE_DIR}/include
     ${ydb-cpp-sdk_BINARY_DIR}
)
mark_targets_as_system("${ydb-cpp-sdk_SOURCE_DIR}")
