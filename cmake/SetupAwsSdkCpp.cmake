include_guard(GLOBAL)

# @ingroup download
option(USERVER_DOWNLOAD_PACKAGE_AWSSDK "Download and setup AWS SDK for C++" ${USERVER_DOWNLOAD_PACKAGES})

set(USERVER_AWSSDK_VERSION 1.11.692)
set(USERVER_AWSSDK_COMPONENTS core sqs)

set(userver_awssdk_use_cpm TRUE)
if(NOT USERVER_FORCE_DOWNLOAD_PACKAGES)
    if(USERVER_DOWNLOAD_PACKAGE_AWSSDK)
        find_package(AWSSDK ${USERVER_AWSSDK_VERSION} QUIET CONFIG COMPONENTS ${USERVER_AWSSDK_COMPONENTS})
    else()
        find_package(AWSSDK ${USERVER_AWSSDK_VERSION} REQUIRED CONFIG COMPONENTS ${USERVER_AWSSDK_COMPONENTS})
    endif()

    if(AWSSDK_FOUND)
        set(userver_awssdk_use_cpm FALSE)
    endif()
endif()

if(userver_awssdk_use_cpm)
    include(SetupCURL)
    find_package(OpenSSL REQUIRED)
    find_package(ZLIB REQUIRED)

    include(DownloadUsingCPM)

    function(_userver_awssdk_set_cache_option NAME VALUE TYPE)
        set(${NAME}
            ${VALUE}
            CACHE ${TYPE} "userver aws-sdk-cpp setup option" FORCE
        )
    endfunction()

    _userver_awssdk_set_cache_option(BUILD_ONLY "sqs" STRING)
    _userver_awssdk_set_cache_option(BUILD_SHARED_LIBS OFF BOOL)
    _userver_awssdk_set_cache_option(BUILD_DEPS ON BOOL)
    _userver_awssdk_set_cache_option(ENABLE_TESTING OFF BOOL)
    _userver_awssdk_set_cache_option(AUTORUN_UNIT_TESTS OFF BOOL)
    _userver_awssdk_set_cache_option(ENABLE_UNITY_BUILD ON BOOL)
    _userver_awssdk_set_cache_option(USE_OPENSSL ON BOOL)
    _userver_awssdk_set_cache_option(ENABLE_OPENSSL_ENCRYPTION ON BOOL)
    _userver_awssdk_set_cache_option(MINIMIZE_SIZE ON BOOL)
    _userver_awssdk_set_cache_option(CMAKE_SKIP_INSTALL_RULES ON BOOL)

    cpmaddpackage(
        NAME aws-sdk-cpp
        VERSION ${USERVER_AWSSDK_VERSION}
        GITHUB_REPOSITORY aws/aws-sdk-cpp
        GIT_TAG ${USERVER_AWSSDK_VERSION}
        GIT_SHALLOW TRUE
    )

    _list_subdirectories("${aws-sdk-cpp_SOURCE_DIR}" userver_awssdk_subdirectories)
    foreach(subdirectory IN LISTS userver_awssdk_subdirectories)
        get_property(
            userver_awssdk_targets
            DIRECTORY "${subdirectory}"
            PROPERTY BUILDSYSTEM_TARGETS
        )
        foreach(target IN LISTS userver_awssdk_targets)
            get_target_property(target_type "${target}" TYPE)
            if(target_type MATCHES "^(STATIC|SHARED|MODULE|OBJECT)_LIBRARY$|^EXECUTABLE$")
                target_compile_options("${target}" PRIVATE -w)
            endif()
        endforeach()
    endforeach()
endif()

if(TARGET aws-cpp-sdk-core AND NOT TARGET AWS::aws-sdk-cpp-core)
    add_library(AWS::aws-sdk-cpp-core ALIAS aws-cpp-sdk-core)
endif()

if(TARGET aws-cpp-sdk-sqs AND NOT TARGET AWS::aws-sdk-cpp-sqs)
    add_library(AWS::aws-sdk-cpp-sqs ALIAS aws-cpp-sdk-sqs)
endif()

if(NOT TARGET AWS::aws-sdk-cpp-core OR NOT TARGET AWS::aws-sdk-cpp-sqs)
    message(
        FATAL_ERROR
            "AWS SDK for C++ was found or downloaded, but expected targets "
            "AWS::aws-sdk-cpp-core and AWS::aws-sdk-cpp-sqs are missing. "
            "Install AWS SDK for C++ with the 'core' and 'sqs' components, or "
            "enable USERVER_DOWNLOAD_PACKAGE_AWSSDK."
    )
endif()
