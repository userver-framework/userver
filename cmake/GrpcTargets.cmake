# Functions to create a target consisting of generated GRPC files and their
# wrappers. A separate target is required as GRPC generated headers require
# relaxed compilation flags, and should be included to the main target as
# 'SYSTEM'.

find_program(PROTOC NAMES yandex-taxi-protoc protoc)
find_program(GRPC_GEN grpc_cpp_plugin)
find_program(
  USRV_GEN protoc_gen_usrv
  PATHS ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/userver
  PATH_SUFFIXES scripts/grpc
)

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
  foreach (proto_file ${GEN_RPC_PROTOS})
    if(NOT IS_ABSOLUTE ${proto_file})
      set(var_name ${proto_file})
      find_path(${var_name} ${proto_file}
        PATHS ${GEN_RPC_SOURCE_PATH}/proto      # For out-of source generated targets
              ${CMAKE_CURRENT_SOURCE_DIR}/proto # For manually added targets
              ${CMAKE_SOURCE_DIR}schemas/proto  # For potentially shared proto files
        DOC "Root path for proto file ${proto_file}"
        NO_DEFAULT_PATH)
      set(root_path ${${var_name}})
      set(proto_file ${root_path}/${proto_file})
    else()
      get_filename_component(root_path ${proto_file} DIRECTORY)
    endif()
    
    get_filename_component(path ${proto_file} DIRECTORY)
    get_filename_component(name_base ${proto_file} NAME_WE)
    file(RELATIVE_PATH rel_path ${root_path} ${path})
    message(STATUS "Root path for ${proto_file} is ${root_path}. Rel path ${rel_path}")

    if(rel_path)
      set(protobuf_header "${rel_path}/${name_base}.pb.h")
      set(protobuf_source "${rel_path}/${name_base}.pb.cc")
      set(grpc_header "${rel_path}/${name_base}.grpc.pb.h")
      set(grpc_source "${rel_path}/${name_base}.grpc.pb.cc")
      set(usrv_header "${rel_path}/${name_base}.usrv.pb.hpp")
    else()
      set(protobuf_header "${name_base}.pb.h")
      set(protobuf_source "${name_base}.pb.cc")
      set(grpc_header "${name_base}.grpc.pb.h")
      set(grpc_source "${name_base}.grpc.pb.cc")
      set(usrv_header "${name_base}.usrv.pb.hpp")
    endif()

    set(files 
      ${GENERATED_PROTO_DIR}/${protobuf_header}
      ${GENERATED_PROTO_DIR}/${protobuf_source}
      ${GENERATED_PROTO_DIR}/${grpc_header}
      ${GENERATED_PROTO_DIR}/${grpc_source}
      ${GENERATED_PROTO_DIR}/${usrv_header})

    add_custom_command(
      OUTPUT ${GENERATED_PROTO_DIR}/${protobuf_header}
             ${GENERATED_PROTO_DIR}/${protobuf_source}
      COMMAND mkdir -p proto
      COMMAND ${PROTOC} ${include_options} -I ${root_path}
              --cpp_out=${GENERATED_PROTO_DIR} ${proto_file}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${proto_file}
      COMMENT "Generate protobuf sources for ${rel_path}/${name_base}.proto"
    )
    add_custom_command(
      OUTPUT ${GENERATED_PROTO_DIR}/${grpc_header}
             ${GENERATED_PROTO_DIR}/${grpc_source}
      COMMAND mkdir -p proto
      COMMAND ${PROTOC} ${include_options} -I ${root_path}
              --grpc_out=${GENERATED_PROTO_DIR}
              --plugin=protoc-gen-grpc=${GRPC_GEN} ${proto_file}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${proto_file}
      COMMENT "Generate grpc sources for ${rel_path}/${name_base}.proto"
    )
    add_custom_command(
      OUTPUT ${GENERATED_PROTO_DIR}/${usrv_header}
      COMMAND mkdir -p proto
      COMMAND ${PROTOC} ${include_options} -I ${root_path}
              --usrv_out=${GENERATED_PROTO_DIR}
              --plugin=protoc-gen-usrv=${USRV_GEN} ${proto_file}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${proto_file}
      COMMENT "Generate userver grpc wrapper for ${rel_path}/${name_base}.proto"
    )

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
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)

  cmake_parse_arguments(RPC_LIB "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  generate_grpc_files(
    PROTOS ${RPC_LIB_PROTOS}
    INCLUDE_DIRECTORIES ${RPC_LIB_INCLUDE_DIRECTORIES}
    GENERATED_INCLUDES include_paths
    CPP_FILES generated_sources
  )
  add_library(${NAME} STATIC ${generated_sources})
  target_compile_options(${NAME} PUBLIC -Wno-unused-parameter)
  target_include_directories(${NAME}
    SYSTEM
    PUBLIC
    ${include_paths}
    PRIVATE
    $<TARGET_PROPERTY:Grpc,INTERFACE_INCLUDE_DIRECTORIES>
  )
endfunction()
