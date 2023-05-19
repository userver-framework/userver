########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND userver_COMPONENT_NAMES userver::core-internal userver::universal userver::core userver::grpc userver::api-common-protos userver::utest userver::ubench userver::postgresql userver::mongo userver::redis userver::rabbitmq)
list(REMOVE_DUPLICATES userver_COMPONENT_NAMES)
list(APPEND userver_FIND_DEPENDENCY_NAMES fmt cctz Boost concurrentqueue yaml-cpp cryptopp jemalloc OpenSSL Boost libev http_parser CURL c-ares gRPC GTest benchmark PostgreSQL mongoc-1.0 hiredis amqpcpp)
list(REMOVE_DUPLICATES userver_FIND_DEPENDENCY_NAMES)
set(Boost_FIND_MODE "NO_MODULE")
set(c-ares_FIND_MODE "NO_MODULE")
set(cctz_FIND_MODE "NO_MODULE")
set(concurrentqueue_FIND_MODE "NO_MODULE")
set(cryptopp_FIND_MODE "NO_MODULE")
set(fmt_FIND_MODE "NO_MODULE")
set(libnghttp2_FIND_MODE "NO_MODULE")
set(CURL_FIND_MODE "NO_MODULE")
set(libev_FIND_MODE "NO_MODULE")
set(http_parser_FIND_MODE "NO_MODULE")
set(OpenSSL_FIND_MODE "NO_MODULE")
set(RapidJSON_FIND_MODE "NO_MODULE")
set(spdlog_FIND_MODE "NO_MODULE")
set(yaml-cpp_FIND_MODE "NO_MODULE")
set(ZLIB_FIND_MODE "NO_MODULE")
set(jemalloc_FIND_MODE "NO_MODULE")
set(gRPC_FIND_MODE "NO_MODULE")
set(PostgreSQL_FIND_MODE "NO_MODULE")
set(mongoc-1.0_FIND_MODE "NO_MODULE")
set(hiredis_FIND_MODE "NO_MODULE")
set(amqpcpp_FIND_MODE "NO_MODULE")
set(GTest_FIND_MODE "NO_MODULE")
set(benchmark_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(userver_PACKAGE_FOLDER_RELEASE "/Users/alexiprof/.conan/data/userver/1.0.0/_/_/package/03147994423ced861fa4eb8ff32bb878d57d8d7e")
set(userver_BUILD_MODULES_PATHS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/cmake/UserverTestsuite.cmake"
			"${userver_PACKAGE_FOLDER_RELEASE}/cmake/AddGoogleTests.cmake"
			"${userver_PACKAGE_FOLDER_RELEASE}/cmake/GrpcConan.cmake"
			"${userver_PACKAGE_FOLDER_RELEASE}/cmake/GrpcTargets.cmake")


set(userver_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/rabbitmq"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/redis"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/mongo"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/postgresql"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/utest"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/api-common-protos"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/grpc"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/core"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/shared"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/universal")
set(userver_RES_DIRS_RELEASE )
set(userver_DEFINITIONS_RELEASE "-DUSERVER_NAMESPACE=userver"
			"-DUSERVER_NAMESPACE_BEGIN=namespace userver {"
			"-DUSERVER_NAMESPACE_END=}")
set(userver_SHARED_LINK_FLAGS_RELEASE )
set(userver_EXE_LINK_FLAGS_RELEASE )
set(userver_OBJECTS_RELEASE )
set(userver_COMPILE_DEFINITIONS_RELEASE "USERVER_NAMESPACE=userver"
			"USERVER_NAMESPACE_BEGIN=namespace userver {"
			"USERVER_NAMESPACE_END=}")
set(userver_COMPILE_OPTIONS_C_RELEASE )
set(userver_COMPILE_OPTIONS_CXX_RELEASE )
set(userver_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_LIBS_RELEASE userver-rabbitmq userver-redis userver-mongo userver-postgresql userver-ubench userver-utest userver-api-common-protos userver-grpc userver-core userver-core-internal userver-universal)
set(userver_SYSTEM_LIBS_RELEASE )
set(userver_FRAMEWORK_DIRS_RELEASE )
set(userver_FRAMEWORKS_RELEASE )
set(userver_BUILD_DIRS_RELEASE )

# COMPOUND VARIABLES
set(userver_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_COMPILE_OPTIONS_C_RELEASE}>")
set(userver_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_EXE_LINK_FLAGS_RELEASE}>")


set(userver_COMPONENTS_RELEASE userver::core-internal userver::universal userver::core userver::grpc userver::api-common-protos userver::utest userver::ubench userver::postgresql userver::mongo userver::redis userver::rabbitmq)
########### COMPONENT userver::rabbitmq VARIABLES ############################################

set(userver_userver_rabbitmq_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/rabbitmq")
set(userver_userver_rabbitmq_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_rabbitmq_RES_DIRS_RELEASE )
set(userver_userver_rabbitmq_DEFINITIONS_RELEASE )
set(userver_userver_rabbitmq_OBJECTS_RELEASE )
set(userver_userver_rabbitmq_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_rabbitmq_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_rabbitmq_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_rabbitmq_LIBS_RELEASE userver-rabbitmq)
set(userver_userver_rabbitmq_SYSTEM_LIBS_RELEASE )
set(userver_userver_rabbitmq_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_rabbitmq_FRAMEWORKS_RELEASE )
set(userver_userver_rabbitmq_DEPENDENCIES_RELEASE userver::core amqpcpp)
set(userver_userver_rabbitmq_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_rabbitmq_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_rabbitmq_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_rabbitmq_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_rabbitmq_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_rabbitmq_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_rabbitmq_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_rabbitmq_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_rabbitmq_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::redis VARIABLES ############################################

set(userver_userver_redis_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/redis")
set(userver_userver_redis_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_redis_RES_DIRS_RELEASE )
set(userver_userver_redis_DEFINITIONS_RELEASE )
set(userver_userver_redis_OBJECTS_RELEASE )
set(userver_userver_redis_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_redis_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_redis_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_redis_LIBS_RELEASE userver-redis)
set(userver_userver_redis_SYSTEM_LIBS_RELEASE )
set(userver_userver_redis_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_redis_FRAMEWORKS_RELEASE )
set(userver_userver_redis_DEPENDENCIES_RELEASE userver::core hiredis::hiredis)
set(userver_userver_redis_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_redis_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_redis_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_redis_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_redis_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_redis_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_redis_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_redis_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_redis_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::mongo VARIABLES ############################################

set(userver_userver_mongo_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/mongo")
set(userver_userver_mongo_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_mongo_RES_DIRS_RELEASE )
set(userver_userver_mongo_DEFINITIONS_RELEASE )
set(userver_userver_mongo_OBJECTS_RELEASE )
set(userver_userver_mongo_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_mongo_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_mongo_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_mongo_LIBS_RELEASE userver-mongo)
set(userver_userver_mongo_SYSTEM_LIBS_RELEASE )
set(userver_userver_mongo_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_mongo_FRAMEWORKS_RELEASE )
set(userver_userver_mongo_DEPENDENCIES_RELEASE userver::core mongo::mongoc_static)
set(userver_userver_mongo_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_mongo_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_mongo_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_mongo_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_mongo_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_mongo_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_mongo_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_mongo_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_mongo_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::postgresql VARIABLES ############################################

set(userver_userver_postgresql_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/postgresql")
set(userver_userver_postgresql_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_postgresql_RES_DIRS_RELEASE )
set(userver_userver_postgresql_DEFINITIONS_RELEASE )
set(userver_userver_postgresql_OBJECTS_RELEASE )
set(userver_userver_postgresql_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_postgresql_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_postgresql_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_postgresql_LIBS_RELEASE userver-postgresql)
set(userver_userver_postgresql_SYSTEM_LIBS_RELEASE )
set(userver_userver_postgresql_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_postgresql_FRAMEWORKS_RELEASE )
set(userver_userver_postgresql_DEPENDENCIES_RELEASE userver::core libpq::pq)
set(userver_userver_postgresql_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_postgresql_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_postgresql_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_postgresql_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_postgresql_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_postgresql_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_postgresql_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_postgresql_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_postgresql_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::ubench VARIABLES ############################################

set(userver_userver_ubench_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include")
set(userver_userver_ubench_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_ubench_RES_DIRS_RELEASE )
set(userver_userver_ubench_DEFINITIONS_RELEASE )
set(userver_userver_ubench_OBJECTS_RELEASE )
set(userver_userver_ubench_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_ubench_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_ubench_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_ubench_LIBS_RELEASE userver-ubench)
set(userver_userver_ubench_SYSTEM_LIBS_RELEASE )
set(userver_userver_ubench_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_ubench_FRAMEWORKS_RELEASE )
set(userver_userver_ubench_DEPENDENCIES_RELEASE userver::core benchmark::benchmark_main)
set(userver_userver_ubench_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_ubench_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_ubench_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_ubench_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_ubench_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_ubench_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_ubench_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_ubench_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_ubench_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::utest VARIABLES ############################################

set(userver_userver_utest_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/utest")
set(userver_userver_utest_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_utest_RES_DIRS_RELEASE )
set(userver_userver_utest_DEFINITIONS_RELEASE )
set(userver_userver_utest_OBJECTS_RELEASE )
set(userver_userver_utest_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_utest_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_utest_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_utest_LIBS_RELEASE userver-utest)
set(userver_userver_utest_SYSTEM_LIBS_RELEASE )
set(userver_userver_utest_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_utest_FRAMEWORKS_RELEASE )
set(userver_userver_utest_DEPENDENCIES_RELEASE userver::core gtest::gtest)
set(userver_userver_utest_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_utest_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_utest_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_utest_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_utest_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_utest_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_utest_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_utest_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_utest_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::api-common-protos VARIABLES ############################################

set(userver_userver_api-common-protos_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/api-common-protos")
set(userver_userver_api-common-protos_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_api-common-protos_RES_DIRS_RELEASE )
set(userver_userver_api-common-protos_DEFINITIONS_RELEASE )
set(userver_userver_api-common-protos_OBJECTS_RELEASE )
set(userver_userver_api-common-protos_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_api-common-protos_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_api-common-protos_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_api-common-protos_LIBS_RELEASE userver-api-common-protos)
set(userver_userver_api-common-protos_SYSTEM_LIBS_RELEASE )
set(userver_userver_api-common-protos_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_api-common-protos_FRAMEWORKS_RELEASE )
set(userver_userver_api-common-protos_DEPENDENCIES_RELEASE userver::grpc)
set(userver_userver_api-common-protos_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_api-common-protos_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_api-common-protos_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_api-common-protos_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_api-common-protos_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_api-common-protos_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_api-common-protos_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_api-common-protos_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_api-common-protos_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::grpc VARIABLES ############################################

set(userver_userver_grpc_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/grpc")
set(userver_userver_grpc_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_grpc_RES_DIRS_RELEASE )
set(userver_userver_grpc_DEFINITIONS_RELEASE )
set(userver_userver_grpc_OBJECTS_RELEASE )
set(userver_userver_grpc_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_grpc_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_grpc_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_grpc_LIBS_RELEASE userver-grpc)
set(userver_userver_grpc_SYSTEM_LIBS_RELEASE )
set(userver_userver_grpc_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_grpc_FRAMEWORKS_RELEASE )
set(userver_userver_grpc_DEPENDENCIES_RELEASE userver::core grpc::grpc)
set(userver_userver_grpc_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_grpc_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_grpc_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_grpc_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_grpc_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_grpc_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_grpc_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_grpc_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_grpc_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::core VARIABLES ############################################

set(userver_userver_core_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/core"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/shared")
set(userver_userver_core_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_core_RES_DIRS_RELEASE )
set(userver_userver_core_DEFINITIONS_RELEASE )
set(userver_userver_core_OBJECTS_RELEASE )
set(userver_userver_core_COMPILE_DEFINITIONS_RELEASE )
set(userver_userver_core_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_core_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_core_LIBS_RELEASE userver-core userver-core-internal)
set(userver_userver_core_SYSTEM_LIBS_RELEASE )
set(userver_userver_core_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_core_FRAMEWORKS_RELEASE )
set(userver_userver_core_DEPENDENCIES_RELEASE userver::core-internal fmt::fmt cctz::cctz boost::boost concurrentqueue::concurrentqueue yaml-cpp libev::libev http_parser::http_parser CURL::libcurl cryptopp::cryptopp jemalloc::jemalloc c-ares::cares)
set(userver_userver_core_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_core_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_core_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_core_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_core_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_core_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_core_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_core_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_core_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::universal VARIABLES ############################################

set(userver_userver_universal_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/universal"
			"${userver_PACKAGE_FOLDER_RELEASE}/include/shared")
set(userver_userver_universal_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_universal_RES_DIRS_RELEASE )
set(userver_userver_universal_DEFINITIONS_RELEASE "-DUSERVER_NAMESPACE=userver"
			"-DUSERVER_NAMESPACE_BEGIN=namespace userver {"
			"-DUSERVER_NAMESPACE_END=}")
set(userver_userver_universal_OBJECTS_RELEASE )
set(userver_userver_universal_COMPILE_DEFINITIONS_RELEASE "USERVER_NAMESPACE=userver"
			"USERVER_NAMESPACE_BEGIN=namespace userver {"
			"USERVER_NAMESPACE_END=}")
set(userver_userver_universal_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_universal_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_universal_LIBS_RELEASE userver-universal)
set(userver_userver_universal_SYSTEM_LIBS_RELEASE )
set(userver_userver_universal_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_universal_FRAMEWORKS_RELEASE )
set(userver_userver_universal_DEPENDENCIES_RELEASE fmt::fmt cctz::cctz boost::boost concurrentqueue::concurrentqueue yaml-cpp cryptopp::cryptopp jemalloc::jemalloc openssl::openssl)
set(userver_userver_universal_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_universal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_universal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_universal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_universal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_universal_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_universal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_universal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_universal_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT userver::core-internal VARIABLES ############################################

set(userver_userver_core-internal_INCLUDE_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/include")
set(userver_userver_core-internal_LIB_DIRS_RELEASE "${userver_PACKAGE_FOLDER_RELEASE}/lib")
set(userver_userver_core-internal_RES_DIRS_RELEASE )
set(userver_userver_core-internal_DEFINITIONS_RELEASE "-DUSERVER_NAMESPACE=userver"
			"-DUSERVER_NAMESPACE_BEGIN=namespace userver {"
			"-DUSERVER_NAMESPACE_END=}")
set(userver_userver_core-internal_OBJECTS_RELEASE )
set(userver_userver_core-internal_COMPILE_DEFINITIONS_RELEASE "USERVER_NAMESPACE=userver"
			"USERVER_NAMESPACE_BEGIN=namespace userver {"
			"USERVER_NAMESPACE_END=}")
set(userver_userver_core-internal_COMPILE_OPTIONS_C_RELEASE "")
set(userver_userver_core-internal_COMPILE_OPTIONS_CXX_RELEASE "")
set(userver_userver_core-internal_LIBS_RELEASE )
set(userver_userver_core-internal_SYSTEM_LIBS_RELEASE )
set(userver_userver_core-internal_FRAMEWORK_DIRS_RELEASE )
set(userver_userver_core-internal_FRAMEWORKS_RELEASE )
set(userver_userver_core-internal_DEPENDENCIES_RELEASE )
set(userver_userver_core-internal_SHARED_LINK_FLAGS_RELEASE )
set(userver_userver_core-internal_EXE_LINK_FLAGS_RELEASE )
# COMPOUND VARIABLES
set(userver_userver_core-internal_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${userver_userver_core-internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${userver_userver_core-internal_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${userver_userver_core-internal_EXE_LINK_FLAGS_RELEASE}>
)
set(userver_userver_core-internal_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${userver_userver_core-internal_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${userver_userver_core-internal_COMPILE_OPTIONS_C_RELEASE}>")