include(FindPackageHandleStandardArgs)

if(NOT gRPC_VERSION)
    set(gRPC_VERSION "${_userver_ydb_grpc_version}")
endif()

if(NOT TARGET gRPC::grpc++ OR NOT gRPC_VERSION)
    if(gRPC_FIND_VERSION_EXACT)
        find_package(gRPC ${gRPC_FIND_VERSION} EXACT QUIET CONFIG)
    elseif(gRPC_FIND_VERSION)
        find_package(gRPC ${gRPC_FIND_VERSION} QUIET CONFIG)
    else()
        find_package(gRPC QUIET CONFIG)
    endif()
endif()

set(_userver_grpc_target_found FALSE)
if(TARGET gRPC::grpc++)
    set(_userver_grpc_target_found TRUE)

    if(NOT TARGET gRPC::grpc_cpp_plugin)
        get_property(_userver_grpc_cpp_plugin GLOBAL PROPERTY userver_grpc_cpp_plugin)
        if(NOT _userver_grpc_cpp_plugin)
            find_program(_userver_grpc_cpp_plugin grpc_cpp_plugin)
        endif()
        if(_userver_grpc_cpp_plugin)
            add_executable(gRPC::grpc_cpp_plugin IMPORTED GLOBAL)
            set_target_properties(gRPC::grpc_cpp_plugin PROPERTIES IMPORTED_LOCATION "${_userver_grpc_cpp_plugin}")
        endif()
    endif()
endif()

find_package_handle_standard_args(
    gRPC
    REQUIRED_VARS _userver_grpc_target_found
    VERSION_VAR gRPC_VERSION
)
