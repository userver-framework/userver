# Functions to create a target consisting of generated GRPC files and their
# wrappers. A separate target is required as GRPC generated headers require
# relaxed compilation flags.

if (USERVER_CONAN)
  find_package(gRPC REQUIRED)
  find_package(Protobuf REQUIRED)
  set(GRPC_LIBRARY_VERSION "${gRPC_VERSION}")
  set(GRPC_PROTOBUF_INCLUDE_DIRS "${protobuf_INCLUDE_DIR}" CACHE PATH INTERNAL)
else()
  find_package(UserverGrpc REQUIRED)
  find_package(UserverProtobuf REQUIRED)
  add_library(Grpc ALIAS UserverGrpc)  # Unify link names
  add_library(Protobuf ALIAS UserverProtobuf)  # Unify link names
  set(GRPC_LIBRARY_VERSION "${UserverGrpc_VERSION}")
  set(GRPC_PROTOBUF_INCLUDE_DIRS "${UserverProtobuf_INCLUDE_DIRS}" CACHE PATH INTERNAL)
endif()

if (NOT GRPC_PROTOBUF_INCLUDE_DIRS)
  message(FATAL_ERROR "Invalid Protobuf package")
endif()
if (NOT GRPC_LIBRARY_VERSION)
  message(FATAL_ERROR "Invalid gRPC package")
endif()

get_filename_component(USERVER_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
set(PROTO_GRPC_USRV_PLUGIN "${USERVER_DIR}/scripts/grpc/protoc_usrv_plugin.sh")

# We only check the system pip protobuf package version once.
if(NOT USERVER_IMPL_GRPC_REQUIREMENTS_CHECKED)
  execute_process(
    COMMAND "${PYTHON}"
      -m pip install --disable-pip-version-check
      -r "${USERVER_DIR}/scripts/grpc/requirements.txt"
    RESULT_VARIABLE RESULT
    WORKING_DIRECTORY "${USERVER_DIR}"
  )
  if(RESULT)
    message(FATAL_ERROR "Protobuf requirements check failed: PYTHON "${PYTHON}" USERVER_DIR ${USERVER_DIR} RESULT ${RESULT}")
  endif(RESULT)
  set(USERVER_IMPL_GRPC_REQUIREMENTS_CHECKED ON CACHE INTERNAL "")
endif()

if(USERVER_CONAN)
  # Can't use find_*, because it may find a system binary with a wrong version.
  set(PROTOBUF_PROTOC "${Protobuf_PROTOC_EXECUTABLE}")
  set(PROTO_GRPC_CPP_PLUGIN "${GRPC_CPP_PLUGIN_PROGRAM}")
else()
  find_program(PROTOBUF_PROTOC NAMES protoc)
  find_program(PROTO_GRPC_CPP_PLUGIN grpc_cpp_plugin)
endif()

if(NOT PROTOBUF_PROTOC)
  message(FATAL_ERROR "protoc not found")
endif()
if(NOT PROTO_GRPC_CPP_PLUGIN)
  message(FATAL_ERROR "grpc_cpp_plugin not found")
endif()

function(generate_grpc_files)
  set(options)
  set(one_value_args CPP_FILES CPP_USRV_FILES GENERATED_INCLUDES SOURCE_PATH)
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(GEN_RPC "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(GEN_RPC_INCLUDE_DIRECTORIES)
    set(include_options)
    foreach(include ${GEN_RPC_INCLUDE_DIRECTORIES})
      if(NOT IS_ABSOLUTE ${include})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${include})
          set(include ${CMAKE_CURRENT_SOURCE_DIR}/${include})
        elseif(EXISTS ${CMAKE_SOURCE_DIR}/${include})
          set(include ${CMAKE_SOURCE_DIR}/${include})
        endif()
      endif()
      get_filename_component(include "${include}" REALPATH BASE_DIR "/")
      if(EXISTS ${include})
        list(APPEND include_options -I ${include})
      else()
        message(WARNING "Include directory ${include} for protoc generator not found")
      endif()
    endforeach()
  endif()

  set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
  get_filename_component(GENERATED_PROTO_DIR "${GENERATED_PROTO_DIR}" REALPATH BASE_DIR "/")

  if(NOT "${GEN_RPC_SOURCE_PATH}" STREQUAL "")
    if(NOT IS_ABSOLUTE ${GEN_RPC_SOURCE_PATH})
      message(SEND_ERROR "SOURCE_PATH='${GEN_RPC_SOURCE_PATH}' is a relative path, which is unsupported.")
    endif()
    set(root_path "${GEN_RPC_SOURCE_PATH}")
  else()
    set(root_path "${CMAKE_CURRENT_SOURCE_DIR}/proto")
  endif()

  get_filename_component(root_path "${root_path}" REALPATH BASE_DIR "/")
  message(STATUS "Generating sources for protos in ${root_path}:")

  set(proto_dependencies_globs ${GEN_RPC_INCLUDE_DIRECTORIES})
  list(TRANSFORM proto_dependencies_globs APPEND "/*.proto")
  list(APPEND proto_dependencies_globs
    "${root_path}/*.proto"
    "${GRPC_PROTOBUF_INCLUDE_DIRS}/*.proto"
    "${USERVER_DIR}/scripts/grpc/*"
  )
  file(GLOB_RECURSE proto_dependencies ${proto_dependencies_globs})
  list(GET proto_dependencies 0 newest_proto_dependency)
  foreach(dependency ${proto_dependencies})
    if("${dependency}" IS_NEWER_THAN "${newest_proto_dependency}")
      set(newest_proto_dependency "${dependency}")
    endif()
  endforeach()

  foreach (proto_file ${GEN_RPC_PROTOS})
    get_filename_component(proto_file "${proto_file}" REALPATH BASE_DIR "${root_path}")

    get_filename_component(path ${proto_file} DIRECTORY)
    get_filename_component(name_base ${proto_file} NAME_WE)
    file(RELATIVE_PATH rel_path "${root_path}" "${path}")

    if(rel_path)
      set(path_base "${rel_path}/${name_base}")
    else()
      set(path_base "${name_base}")
    endif()

    set(did_generate_proto_sources FALSE)
    if("${newest_proto_dependency}" IS_NEWER_THAN "${GENERATED_PROTO_DIR}/${path_base}.pb.cc")
      execute_process(
        COMMAND mkdir -p proto
        COMMAND ${PROTOBUF_PROTOC} ${include_options}
              --cpp_out=${GENERATED_PROTO_DIR}
              --grpc_out=${GENERATED_PROTO_DIR}
              --usrv_out=${GENERATED_PROTO_DIR}
              -I ${root_path}
              -I ${GRPC_PROTOBUF_INCLUDE_DIRS}
              --plugin=protoc-gen-grpc=${PROTO_GRPC_CPP_PLUGIN}
              --plugin=protoc-gen-usrv=${PROTO_GRPC_USRV_PLUGIN}
              ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE execute_process_result
      )
      if(execute_process_result)
        message(SEND_ERROR "Error while generating gRPC sources for ${path_base}.proto")
      else()
        set(did_generate_proto_sources TRUE)
      endif()
    else()
      message(STATUS "Reused previously generated sources for ${path_base}.proto")
    endif()

    set(files
      ${GENERATED_PROTO_DIR}/${path_base}.pb.h
      ${GENERATED_PROTO_DIR}/${path_base}.pb.cc
    )

    if (EXISTS ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.hpp)
      if(did_generate_proto_sources)
        message(STATUS "Generated sources for ${path_base}.proto with gRPC")
      endif()

      set(usrv_files
        ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.cpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.usrv.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.usrv.pb.cpp
      )
      list(APPEND files
        ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.h
        ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.cc
      )
    elseif(did_generate_proto_sources)
      message(STATUS "Generated sources for ${path_base}.proto")
    endif()

    set_source_files_properties(${files} ${usrv_files} PROPERTIES GENERATED 1)
    list(APPEND generated_cpps ${files})
    list(APPEND generated_usrv_cpps ${usrv_files})
  endforeach()

  if(GEN_RPC_GENERATED_INCLUDES)
    set(${GEN_RPC_GENERATED_INCLUDES} ${GENERATED_PROTO_DIR} PARENT_SCOPE)
  endif()
  if(GEN_RPC_CPP_FILES)
    set(${GEN_RPC_CPP_FILES} ${generated_cpps} PARENT_SCOPE)
  endif()
  if(GEN_RPC_CPP_USRV_FILES)
    set(${GEN_RPC_CPP_USRV_FILES} ${generated_usrv_cpps} PARENT_SCOPE)
  endif()
endfunction()

function(add_grpc_library NAME)
  set(options)
  set(one_value_args SOURCE_PATH)
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(RPC_LIB "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  generate_grpc_files(
    PROTOS ${RPC_LIB_PROTOS}
    INCLUDE_DIRECTORIES ${RPC_LIB_INCLUDE_DIRECTORIES}
    SOURCE_PATH ${RPC_LIB_SOURCE_PATH}
    GENERATED_INCLUDES include_paths
    CPP_FILES generated_sources
    CPP_USRV_FILES generated_usrv_sources
  )
  add_library(${NAME} STATIC ${generated_sources} ${generated_usrv_sources})
  target_compile_options(${NAME} PUBLIC -Wno-unused-parameter)
  target_include_directories(${NAME} SYSTEM PUBLIC ${include_paths})
  if(USERVER_CONAN)
    target_link_libraries(${NAME} PUBLIC userver::grpc)
  else()
    target_link_libraries(${NAME} PUBLIC userver-grpc)
  endif()
endfunction()
