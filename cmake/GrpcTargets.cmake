# Functions to create a target consisting of generated GRPC files and their
# wrappers. A separate target is required as GRPC generated headers require
# relaxed compilation flags.

if(NOT USERVER_OPEN_SOURCE_BUILD)
  find_program(PROTOBUF_PROTOC NAMES yandex-taxi-protoc protoc)
else()
<<<<<<< HEAD
  find_program(PROTOBUF_PROTOC NAMES protoc)
=======
  if(NOT Protobuf_FOUND)
    include(SetupProtobuf)
  endif()
  if(Protobuf_INCLUDE_DIR)
    set(USERVER_PROTOBUF_IMPORT_DIR "${Protobuf_INCLUDE_DIR}")
  else()
    set(USERVER_PROTOBUF_IMPORT_DIR "${Protobuf_INCLUDE_DIRS}")
  endif()

  include(SetupGrpc)
endif()

if (NOT USERVER_PROTOBUF_IMPORT_DIR)
  message(FATAL_ERROR "Invalid Protobuf package")
>>>>>>> workaround for UserverGrpc linkage after install
endif()
find_program(PROTO_GRPC_CPP_PLUGIN grpc_cpp_plugin)

get_filename_component(USERVER_DIR ${CMAKE_CURRENT_LIST_DIR} DIRECTORY)
set(PROTO_GRPC_USRV_PLUGIN ${USERVER_DIR}/scripts/grpc/protoc_usrv_plugin)

function(generate_grpc_files)
  set(options)
  set(one_value_args CPP_FILES GENERATED_INCLUDES SOURCE_PATH)
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
      if(EXISTS ${include})
        list(APPEND include_options -I ${include})
      else()
        message(WARNING "Include directory ${include} for protoc generator not found")
      endif()
    endforeach()
  endif()

  set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)

  if(NOT "${GEN_RPC_SOURCE_PATH}" STREQUAL "")
    if(NOT IS_ABSOLUTE ${GEN_RPC_SOURCE_PATH})
      message(SEND_ERROR "SOURCE_PATH='${GEN_RPC_SOURCE_PATH}' is a relative path, which is unsupported.")
    endif()
    set(root_path "${GEN_RPC_SOURCE_PATH}")
  else()
    set(root_path "${CMAKE_CURRENT_SOURCE_DIR}/proto")
  endif()

  foreach (proto_file ${GEN_RPC_PROTOS})
    if(NOT IS_ABSOLUTE ${proto_file})
      get_filename_component(proto_file "${proto_file}" REALPATH BASE_DIR "${root_path}")
    endif()

    get_filename_component(path ${proto_file} DIRECTORY)
    get_filename_component(name_base ${proto_file} NAME_WE)
    file(RELATIVE_PATH rel_path "${root_path}" "${path}")
    message(STATUS "Root path for ${proto_file} is ${root_path}. Rel path is '${rel_path}'")

    if(rel_path)
      set(path_base "${rel_path}/${name_base}")
    else()
      set(path_base "${name_base}")
    endif()

<<<<<<< HEAD
    # resolve root_path, proto_file to real path's - protoc check that root_path is prefix of proto_file (this can be non true if project inside folder sym linked to other dir)
    get_filename_component(real_root_path ${root_path} REALPATH)
    get_filename_component(real_proto_file ${proto_file} REALPATH)

    execute_process(
      COMMAND mkdir -p proto
      COMMAND ${PROTOBUF_PROTOC}
              -I ${real_root_path} ${include_options}
=======
    set(did_generate_proto_sources FALSE)
    if("${newest_proto_dependency}" IS_NEWER_THAN "${GENERATED_PROTO_DIR}/${path_base}.pb.cc")
      # resolve root_path, proto_file to real path's - protoc check that root_path is prefix of proto_file (this can be non true if project inside folder sym linked to other dir)
      get_filename_component(real_root_path ${root_path} REALPATH)
      get_filename_component(real_proto_file ${proto_file} REALPATH)
      execute_process(
        COMMAND mkdir -p proto
        COMMAND ${PROTOBUF_PROTOC} ${include_options}
>>>>>>> Fix for building inside folder sym linked to other
              --cpp_out=${GENERATED_PROTO_DIR}
              --grpc_out=${GENERATED_PROTO_DIR}
              --usrv_out=${GENERATED_PROTO_DIR}
<<<<<<< HEAD
=======
              --python_out=${GENERATED_PROTO_DIR}
              --grpc_python_out=${GENERATED_PROTO_DIR}
              ${pyi_out_param}
              -I ${root_path}
              -I ${USERVER_PROTOBUF_IMPORT_DIR}
>>>>>>> fix compilation
              --plugin=protoc-gen-grpc=${PROTO_GRPC_CPP_PLUGIN}
              --plugin=protoc-gen-usrv=${PROTO_GRPC_USRV_PLUGIN}
<<<<<<< HEAD
              ${real_proto_file}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      RESULT_VARIABLE execute_process_result
    )
    if(execute_process_result)
      message(SEND_ERROR "Error while generating gRPC sources for ${path_base}.proto")
=======
              --plugin=protoc-gen-grpc_python=${PROTO_GRPC_PYTHON_PLUGIN}
              ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE execute_process_result
      )
      if(execute_process_result)
        message(SEND_ERROR "Error while generating gRPC sources for ${path_base}.proto")
      else()
        file(TOUCH ${CMAKE_CURRENT_BINARY_DIR}/proto/${rel_path}/__init__.py)
        set(did_generate_proto_sources TRUE)
      endif()
>>>>>>> Fix for building inside folder sym linked to other
    else()
      message(STATUS "Generated gRPC sources for ${path_base}.proto")
    endif()

    set(files
      ${GENERATED_PROTO_DIR}/${path_base}.pb.h
      ${GENERATED_PROTO_DIR}/${path_base}.pb.cc
    )

    if(EXISTS ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.h)
      list(APPEND files
        ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.h
        ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.cc
      )
    endif()

    if (EXISTS ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.hpp)
      list(APPEND files
        ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.cpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.usrv.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.usrv.pb.cpp
      )
    endif()

    set_source_files_properties(${files} PROPERTIES GENERATED 1)
    list(APPEND generated_cpps ${files})
  endforeach()

  if(GEN_RPC_GENERATED_INCLUDES)
    set(${GEN_RPC_GENERATED_INCLUDES} ${GENERATED_PROTO_DIR} PARENT_SCOPE)
  endif()
  if(GEN_RPC_CPP_FILES)
    set(${GEN_RPC_CPP_FILES} ${generated_cpps} PARENT_SCOPE)
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
  )
  add_library(${NAME} STATIC ${generated_sources})
  target_compile_options(${NAME} PUBLIC -Wno-unused-parameter)
  target_include_directories(${NAME} SYSTEM PUBLIC ${include_paths})
  target_link_libraries(${NAME} PUBLIC userver-core userver-grpc Protobuf)
endfunction()
