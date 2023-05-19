# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(userver_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(userver_FRAMEWORKS_FOUND_RELEASE "${userver_FRAMEWORKS_RELEASE}" "${userver_FRAMEWORK_DIRS_RELEASE}")

set(userver_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET userver_DEPS_TARGET)
    add_library(userver_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET userver_DEPS_TARGET
             PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${userver_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${userver_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:fmt::fmt;cctz::cctz;boost::boost;concurrentqueue::concurrentqueue;yaml-cpp;cryptopp::cryptopp;jemalloc::jemalloc;openssl::openssl;fmt::fmt;cctz::cctz;boost::boost;concurrentqueue::concurrentqueue;yaml-cpp;libev::libev;http_parser::http_parser;CURL::libcurl;cryptopp::cryptopp;jemalloc::jemalloc;c-ares::cares;userver::core-internal;grpc::grpc;userver::core;userver::grpc;gtest::gtest;userver::core;benchmark::benchmark_main;userver::core;libpq::pq;userver::core;mongo::mongoc_static;userver::core;hiredis::hiredis;userver::core;amqpcpp;userver::core>
             APPEND)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### userver_DEPS_TARGET to all of them
conan_package_library_targets("${userver_LIBS_RELEASE}"    # libraries
                              "${userver_LIB_DIRS_RELEASE}" # package_libdir
                              userver_DEPS_TARGET
                              userver_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "userver")    # package_name

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${userver_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## COMPONENTS TARGET PROPERTIES Release ########################################

    ########## COMPONENT userver::rabbitmq #############

        set(userver_userver_rabbitmq_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_rabbitmq_FRAMEWORKS_FOUND_RELEASE "${userver_userver_rabbitmq_FRAMEWORKS_RELEASE}" "${userver_userver_rabbitmq_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_rabbitmq_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_rabbitmq_DEPS_TARGET)
            add_library(userver_userver_rabbitmq_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_rabbitmq_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_rabbitmq_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_rabbitmq_LIBS_RELEASE}"
                                      "${userver_userver_rabbitmq_LIB_DIRS_RELEASE}"
                                      userver_userver_rabbitmq_DEPS_TARGET
                                      userver_userver_rabbitmq_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_rabbitmq")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::rabbitmq
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_rabbitmq_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::rabbitmq
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_rabbitmq_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::rabbitmq PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::rabbitmq PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::rabbitmq PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::rabbitmq PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_rabbitmq_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::redis #############

        set(userver_userver_redis_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_redis_FRAMEWORKS_FOUND_RELEASE "${userver_userver_redis_FRAMEWORKS_RELEASE}" "${userver_userver_redis_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_redis_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_redis_DEPS_TARGET)
            add_library(userver_userver_redis_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_redis_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_redis_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_redis_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_redis_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_redis_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_redis_LIBS_RELEASE}"
                                      "${userver_userver_redis_LIB_DIRS_RELEASE}"
                                      userver_userver_redis_DEPS_TARGET
                                      userver_userver_redis_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_redis")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::redis
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_redis_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_redis_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_redis_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::redis
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_redis_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::redis PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_redis_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::redis PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_redis_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::redis PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_redis_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::redis PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_redis_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::mongo #############

        set(userver_userver_mongo_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_mongo_FRAMEWORKS_FOUND_RELEASE "${userver_userver_mongo_FRAMEWORKS_RELEASE}" "${userver_userver_mongo_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_mongo_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_mongo_DEPS_TARGET)
            add_library(userver_userver_mongo_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_mongo_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_mongo_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_mongo_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_mongo_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_mongo_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_mongo_LIBS_RELEASE}"
                                      "${userver_userver_mongo_LIB_DIRS_RELEASE}"
                                      userver_userver_mongo_DEPS_TARGET
                                      userver_userver_mongo_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_mongo")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::mongo
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_mongo_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_mongo_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_mongo_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::mongo
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_mongo_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::mongo PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_mongo_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::mongo PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_mongo_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::mongo PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_mongo_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::mongo PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_mongo_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::postgresql #############

        set(userver_userver_postgresql_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_postgresql_FRAMEWORKS_FOUND_RELEASE "${userver_userver_postgresql_FRAMEWORKS_RELEASE}" "${userver_userver_postgresql_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_postgresql_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_postgresql_DEPS_TARGET)
            add_library(userver_userver_postgresql_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_postgresql_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_postgresql_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_postgresql_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_postgresql_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_postgresql_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_postgresql_LIBS_RELEASE}"
                                      "${userver_userver_postgresql_LIB_DIRS_RELEASE}"
                                      userver_userver_postgresql_DEPS_TARGET
                                      userver_userver_postgresql_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_postgresql")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::postgresql
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_postgresql_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_postgresql_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_postgresql_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::postgresql
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_postgresql_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::postgresql PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_postgresql_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::postgresql PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_postgresql_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::postgresql PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_postgresql_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::postgresql PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_postgresql_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::ubench #############

        set(userver_userver_ubench_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_ubench_FRAMEWORKS_FOUND_RELEASE "${userver_userver_ubench_FRAMEWORKS_RELEASE}" "${userver_userver_ubench_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_ubench_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_ubench_DEPS_TARGET)
            add_library(userver_userver_ubench_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_ubench_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_ubench_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_ubench_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_ubench_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_ubench_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_ubench_LIBS_RELEASE}"
                                      "${userver_userver_ubench_LIB_DIRS_RELEASE}"
                                      userver_userver_ubench_DEPS_TARGET
                                      userver_userver_ubench_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_ubench")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::ubench
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_ubench_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_ubench_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_ubench_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::ubench
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_ubench_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::ubench PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_ubench_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::ubench PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_ubench_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::ubench PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_ubench_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::ubench PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_ubench_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::utest #############

        set(userver_userver_utest_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_utest_FRAMEWORKS_FOUND_RELEASE "${userver_userver_utest_FRAMEWORKS_RELEASE}" "${userver_userver_utest_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_utest_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_utest_DEPS_TARGET)
            add_library(userver_userver_utest_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_utest_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_utest_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_utest_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_utest_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_utest_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_utest_LIBS_RELEASE}"
                                      "${userver_userver_utest_LIB_DIRS_RELEASE}"
                                      userver_userver_utest_DEPS_TARGET
                                      userver_userver_utest_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_utest")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::utest
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_utest_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_utest_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_utest_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::utest
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_utest_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::utest PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_utest_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::utest PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_utest_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::utest PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_utest_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::utest PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_utest_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::api-common-protos #############

        set(userver_userver_api-common-protos_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_api-common-protos_FRAMEWORKS_FOUND_RELEASE "${userver_userver_api-common-protos_FRAMEWORKS_RELEASE}" "${userver_userver_api-common-protos_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_api-common-protos_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_api-common-protos_DEPS_TARGET)
            add_library(userver_userver_api-common-protos_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_api-common-protos_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_api-common-protos_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_api-common-protos_LIBS_RELEASE}"
                                      "${userver_userver_api-common-protos_LIB_DIRS_RELEASE}"
                                      userver_userver_api-common-protos_DEPS_TARGET
                                      userver_userver_api-common-protos_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_api-common-protos")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::api-common-protos
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_api-common-protos_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::api-common-protos
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_api-common-protos_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::api-common-protos PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::api-common-protos PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::api-common-protos PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::api-common-protos PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_api-common-protos_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::grpc #############

        set(userver_userver_grpc_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_grpc_FRAMEWORKS_FOUND_RELEASE "${userver_userver_grpc_FRAMEWORKS_RELEASE}" "${userver_userver_grpc_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_grpc_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_grpc_DEPS_TARGET)
            add_library(userver_userver_grpc_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_grpc_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_grpc_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_grpc_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_grpc_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_grpc_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_grpc_LIBS_RELEASE}"
                                      "${userver_userver_grpc_LIB_DIRS_RELEASE}"
                                      userver_userver_grpc_DEPS_TARGET
                                      userver_userver_grpc_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_grpc")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::grpc
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_grpc_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_grpc_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_grpc_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::grpc
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_grpc_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::grpc PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_grpc_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::grpc PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_grpc_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::grpc PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_grpc_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::grpc PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_grpc_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::core #############

        set(userver_userver_core_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_core_FRAMEWORKS_FOUND_RELEASE "${userver_userver_core_FRAMEWORKS_RELEASE}" "${userver_userver_core_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_core_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_core_DEPS_TARGET)
            add_library(userver_userver_core_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_core_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_core_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_core_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_core_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_core_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_core_LIBS_RELEASE}"
                                      "${userver_userver_core_LIB_DIRS_RELEASE}"
                                      userver_userver_core_DEPS_TARGET
                                      userver_userver_core_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_core")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::core
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_core_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_core_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_core_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::core
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_core_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::core PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_core_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::core PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_core_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::core PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_core_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::core PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_core_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::universal #############

        set(userver_userver_universal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_universal_FRAMEWORKS_FOUND_RELEASE "${userver_userver_universal_FRAMEWORKS_RELEASE}" "${userver_userver_universal_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_universal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_universal_DEPS_TARGET)
            add_library(userver_userver_universal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_universal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_universal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_universal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_universal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_universal_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_universal_LIBS_RELEASE}"
                                      "${userver_userver_universal_LIB_DIRS_RELEASE}"
                                      userver_userver_universal_DEPS_TARGET
                                      userver_userver_universal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_universal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::universal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_universal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_universal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_universal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::universal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_universal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::universal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_universal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::universal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_universal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::universal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_universal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::universal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_universal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## COMPONENT userver::core-internal #############

        set(userver_userver_core-internal_FRAMEWORKS_FOUND_RELEASE "")
        conan_find_apple_frameworks(userver_userver_core-internal_FRAMEWORKS_FOUND_RELEASE "${userver_userver_core-internal_FRAMEWORKS_RELEASE}" "${userver_userver_core-internal_FRAMEWORK_DIRS_RELEASE}")

        set(userver_userver_core-internal_LIBRARIES_TARGETS "")

        ######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
        if(NOT TARGET userver_userver_core-internal_DEPS_TARGET)
            add_library(userver_userver_core-internal_DEPS_TARGET INTERFACE IMPORTED)
        endif()

        set_property(TARGET userver_userver_core-internal_DEPS_TARGET
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_core-internal_FRAMEWORKS_FOUND_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_core-internal_SYSTEM_LIBS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_core-internal_DEPENDENCIES_RELEASE}>
                     APPEND)

        ####### Find the libraries declared in cpp_info.component["xxx"].libs,
        ####### create an IMPORTED target for each one and link the 'userver_userver_core-internal_DEPS_TARGET' to all of them
        conan_package_library_targets("${userver_userver_core-internal_LIBS_RELEASE}"
                                      "${userver_userver_core-internal_LIB_DIRS_RELEASE}"
                                      userver_userver_core-internal_DEPS_TARGET
                                      userver_userver_core-internal_LIBRARIES_TARGETS
                                      "_RELEASE"
                                      "userver_userver_core-internal")

        ########## TARGET PROPERTIES #####################################
        set_property(TARGET userver::core-internal
                     PROPERTY INTERFACE_LINK_LIBRARIES
                     $<$<CONFIG:Release>:${userver_userver_core-internal_OBJECTS_RELEASE}>
                     $<$<CONFIG:Release>:${userver_userver_core-internal_LIBRARIES_TARGETS}>
                     APPEND)

        if("${userver_userver_core-internal_LIBS_RELEASE}" STREQUAL "")
            # If the component is not declaring any "cpp_info.components['foo'].libs" the system, frameworks etc are not
            # linked to the imported targets and we need to do it to the global target
            set_property(TARGET userver::core-internal
                         PROPERTY INTERFACE_LINK_LIBRARIES
                         userver_userver_core-internal_DEPS_TARGET
                         APPEND)
        endif()

        set_property(TARGET userver::core-internal PROPERTY INTERFACE_LINK_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_core-internal_LINKER_FLAGS_RELEASE}> APPEND)
        set_property(TARGET userver::core-internal PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                     $<$<CONFIG:Release>:${userver_userver_core-internal_INCLUDE_DIRS_RELEASE}> APPEND)
        set_property(TARGET userver::core-internal PROPERTY INTERFACE_COMPILE_DEFINITIONS
                     $<$<CONFIG:Release>:${userver_userver_core-internal_COMPILE_DEFINITIONS_RELEASE}> APPEND)
        set_property(TARGET userver::core-internal PROPERTY INTERFACE_COMPILE_OPTIONS
                     $<$<CONFIG:Release>:${userver_userver_core-internal_COMPILE_OPTIONS_RELEASE}> APPEND)

    ########## AGGREGATED GLOBAL TARGET WITH THE COMPONENTS #####################
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::rabbitmq APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::redis APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::mongo APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::postgresql APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::ubench APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::utest APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::api-common-protos APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::grpc APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::core APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::universal APPEND)
    set_property(TARGET userver::userver PROPERTY INTERFACE_LINK_LIBRARIES userver::core-internal APPEND)

########## For the modules (FindXXX)
set(userver_LIBRARIES_RELEASE userver::userver)
