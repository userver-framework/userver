# Functions for creating targets consisting of base protobuf files and userver
# asynchronous gRPC adaptors.
#
# On inclusion:
# - finds Protobuf package
# - finds gRPC package
# - sets up project-wide venv-userver-grpc for userver protobuf plugin
#
# Provides:
# - userver_generate_grpc_files function
# - userver_add_grpc_library function
#
# See the Doxygen documentation on add_grpc_library.
#
# Implementation note: public functions here should be usable even without
# a direct include of this script, so the functions should not rely
# on non-cache variables being present.

include_guard()

function(_userver_prepare_grpc)
  if(USERVER_CONAN)
    find_package(gRPC REQUIRED)
    find_package(Protobuf REQUIRED)  # For Protobuf_VERSION
    get_target_property(PROTO_GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
    get_target_property(PROTO_GRPC_PYTHON_PLUGIN gRPC::grpc_python_plugin LOCATION)
    set(PROTOBUF_PROTOC "${Protobuf_PROTOC_EXECUTABLE}")
  else()
    if(NOT Protobuf_FOUND)
      include(SetupProtobuf)
    endif()
    include(SetupGrpc)
  endif()
  set_property(GLOBAL PROPERTY userver_grpc_cpp_plugin "${PROTO_GRPC_CPP_PLUGIN}")
  set_property(GLOBAL PROPERTY userver_grpc_python_plugin "${PROTO_GRPC_PYTHON_PLUGIN}")
  set_property(GLOBAL PROPERTY userver_protobuf_protoc "${PROTOBUF_PROTOC}")

  if(NOT USERVER_GRPC_SCRIPTS_PATH)
    get_filename_component(USERVER_DIR "${CMAKE_CURRENT_LIST_DIR}" DIRECTORY)
    set(USERVER_GRPC_SCRIPTS_PATH "${USERVER_DIR}/scripts/grpc")
  endif()
  set_property(GLOBAL PROPERTY userver_grpc_scripts_path "${USERVER_GRPC_SCRIPTS_PATH}")

  message(STATUS "Protobuf version: ${Protobuf_VERSION}")
  message(STATUS "gRPC version: ${gRPC_VERSION}")

  # Used by grpc/CMakeLists.txt
  set(gRPC_VERSION "${gRPC_VERSION}" PARENT_SCOPE)

  if(Protobuf_INCLUDE_DIR)
    set_property(GLOBAL PROPERTY userver_protobuf_import_dir "${Protobuf_INCLUDE_DIR}")
  elseif(protobuf_INCLUDE_DIR)
    set_property(GLOBAL PROPERTY userver_protobuf_import_dir "${protobuf_INCLUDE_DIR}")
  elseif(Protobuf_INCLUDE_DIRS)
    set_property(GLOBAL PROPERTY userver_protobuf_import_dir "${Protobuf_INCLUDE_DIRS}")
  elseif(protobuf_INCLUDE_DIRS)
    set_property(GLOBAL PROPERTY userver_protobuf_import_dir "${protobuf_INCLUDE_DIRS}")
  else()
    message(FATAL_ERROR "Invalid Protobuf package")
  endif()

  if (NOT gRPC_VERSION)
    message(FATAL_ERROR "Invalid gRPC package")
  endif()
  if(NOT PROTOBUF_PROTOC)
    message(FATAL_ERROR "protoc not found")
  endif()
  if(NOT PROTO_GRPC_CPP_PLUGIN)
    message(FATAL_ERROR "grpc_cpp_plugin not found")
  endif()
  if(NOT PROTO_GRPC_PYTHON_PLUGIN)
    message(FATAL_ERROR "grpc_python_plugin not found")
  endif()

  if(NOT USERVER_CONAN)
    # For Conan builds, UserverTestsuite is included automatically.
    include(UserverTestsuite)
  endif()

  get_property(protobuf_category GLOBAL PROPERTY userver_protobuf_version_category)
  set(requirements_name "requirements-${protobuf_category}.txt")

  userver_venv_setup(
      NAME userver-grpc
      PYTHON_OUTPUT_VAR USERVER_GRPC_PYTHON_BINARY
      REQUIREMENTS "${USERVER_GRPC_SCRIPTS_PATH}/${requirements_name}"
      UNIQUE
  )
  set(ENV{USERVER_GRPC_PYTHON_BINARY} "${USERVER_GRPC_PYTHON_BINARY}")
endfunction()

_userver_prepare_grpc()

function(userver_generate_grpc_files)
  set(options)
  set(one_value_args CPP_FILES CPP_USRV_FILES GENERATED_INCLUDES SOURCE_PATH)
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(GEN_RPC "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  get_property(USERVER_GRPC_SCRIPTS_PATH GLOBAL PROPERTY userver_grpc_scripts_path)
  get_property(PROTO_GRPC_CPP_PLUGIN GLOBAL PROPERTY userver_grpc_cpp_plugin)
  get_property(PROTO_GRPC_PYTHON_PLUGIN GLOBAL PROPERTY userver_grpc_python_plugin)
  get_property(PROTOBUF_PROTOC GLOBAL PROPERTY userver_protobuf_protoc)
  get_property(USERVER_PROTOBUF_IMPORT_DIR GLOBAL PROPERTY userver_protobuf_import_dir)
  set(PROTO_GRPC_USRV_PLUGIN "${USERVER_GRPC_SCRIPTS_PATH}/protoc_usrv_plugin.sh")

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

  set(pyi_out_param "")
  if(Protobuf_VERSION VERSION_GREATER_EQUAL "3.20.0")
    set(pyi_out_param "--pyi_out=${GENERATED_PROTO_DIR}")
  endif()

  set(protoc_flags
      "--cpp_out=${GENERATED_PROTO_DIR}"
      "--grpc_out=${GENERATED_PROTO_DIR}"
      "--usrv_out=${GENERATED_PROTO_DIR}"
      "--python_out=${GENERATED_PROTO_DIR}"
      "--grpc_python_out=${GENERATED_PROTO_DIR}"
      ${pyi_out_param}
      -I "${root_path}"
      -I "${USERVER_PROTOBUF_IMPORT_DIR}"
      ${include_options}
      "--plugin=protoc-gen-grpc=${PROTO_GRPC_CPP_PLUGIN}"
      "--plugin=protoc-gen-usrv=${PROTO_GRPC_USRV_PLUGIN}"
      "--plugin=protoc-gen-grpc_python=${PROTO_GRPC_PYTHON_PLUGIN}"
  )

  set(proto_abs_paths)
  set(proto_rel_paths)
  foreach(proto_file ${GEN_RPC_PROTOS})
    get_filename_component(proto_file "${proto_file}" REALPATH BASE_DIR "${root_path}")
    get_filename_component(path "${proto_file}" DIRECTORY)
    get_filename_component(name_base "${proto_file}" NAME_WE)
    file(RELATIVE_PATH path_base "${root_path}" "${path}/${name_base}")
    list(APPEND proto_abs_paths "${proto_file}")
    list(APPEND proto_rel_paths "${path_base}")
  endforeach()
  set(did_generate_proto_sources FALSE)

  set(proto_dependencies_globs ${GEN_RPC_INCLUDE_DIRECTORIES})
  list(TRANSFORM proto_dependencies_globs APPEND "/*.proto")
  list(APPEND proto_dependencies_globs
      "${root_path}/*.proto"
      "${USERVER_PROTOBUF_IMPORT_DIR}/*.proto"
      "${USERVER_GRPC_SCRIPTS_PATH}/*"
  )
  file(GLOB_RECURSE proto_dependencies ${proto_dependencies_globs})

  set(generated_cpps)
  set(generated_usrv_cpps)
  set(pyi_init_files)

  foreach(proto_rel_path ${proto_rel_paths})
    list(APPEND generated_cpps
        ${GENERATED_PROTO_DIR}/${proto_rel_path}.pb.h
        ${GENERATED_PROTO_DIR}/${proto_rel_path}.pb.cc
    )

    set(has_service_sources FALSE)
    # The files have not been generated yet, so we can't get the information
    # on services from protoc at this stage.
    # HACK: file contains service <=> a line starts with 'service '
    file(STRINGS "${root_path}/${proto_rel_path}.proto" proto_service_string
        REGEX "^service " ENCODING UTF-8)
    if(proto_service_string)
      set(has_service_sources TRUE)
    endif()

    if(has_service_sources)
      list(APPEND generated_usrv_cpps
          ${GENERATED_PROTO_DIR}/${proto_rel_path}_client.usrv.pb.hpp
          ${GENERATED_PROTO_DIR}/${proto_rel_path}_client.usrv.pb.cpp
          ${GENERATED_PROTO_DIR}/${proto_rel_path}_service.usrv.pb.hpp
          ${GENERATED_PROTO_DIR}/${proto_rel_path}_service.usrv.pb.cpp
      )
      list(APPEND generated_cpps
          ${GENERATED_PROTO_DIR}/${proto_rel_path}.grpc.pb.h
          ${GENERATED_PROTO_DIR}/${proto_rel_path}.grpc.pb.cc
      )
    endif()

    if(pyi_out_param)
      get_filename_component(proto_rel_dir "${proto_rel_path}" DIRECTORY)
      set(pyi_init_file "${CMAKE_CURRENT_BINARY_DIR}/proto/${proto_rel_dir}/__init__.py")
      file(WRITE "${pyi_init_file}" "")
      list(APPEND pyi_init_files "${pyi_init_file}")
    endif()
  endforeach()

  if(USERVER_GENERATE_PROTOS_AT_BUILD_TIME)
    add_custom_command(
        OUTPUT ${generated_cpps} ${generated_usrv_cpps}
        COMMAND "${PROTOBUF_PROTOC}" ${protoc_flags} ${proto_abs_paths}
        DEPENDS ${proto_dependencies}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Running gRPC C++ protocol buffer compiler for ${root_path}"
    )
    message(STATUS "Scheduled build-time generation of protos in ${root_path}")
  else()
    list(GET proto_dependencies 0 newest_proto_dependency)
    foreach(dependency ${proto_dependencies})
      if("${dependency}" IS_NEWER_THAN "${newest_proto_dependency}")
        set(newest_proto_dependency "${dependency}")
      endif()
    endforeach()

    set(should_generate_protos FALSE)
    foreach(proto_rel_path ${proto_rel_paths})
      if("${newest_proto_dependency}" IS_NEWER_THAN "${GENERATED_PROTO_DIR}/${proto_rel_path}.pb.cc")
        set(should_generate_protos TRUE)
        break()
      endif()
    endforeach()

    if(should_generate_protos)
      message(STATUS "Generating sources for protos in ${root_path}:")
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto")
      execute_process(
          COMMAND "${PROTOBUF_PROTOC}" ${protoc_flags} ${proto_abs_paths}
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          RESULT_VARIABLE execute_process_result
      )
      if(execute_process_result)
        message(SEND_ERROR "Error while generating gRPC sources for protos in ${root_path}")
      else()
        set(did_generate_proto_sources TRUE)
      endif()
    else()
      message(STATUS "Reused previously generated sources for protos in ${root_path}")
    endif()
  endif()

  set_source_files_properties(
      ${generated_cpps} ${generated_usrv_cpps} ${pyi_init_files}
      PROPERTIES GENERATED 1
  )

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

function(userver_add_grpc_library NAME)
  set(options)
  set(one_value_args SOURCE_PATH)
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(RPC_LIB "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  userver_generate_grpc_files(
      PROTOS ${RPC_LIB_PROTOS}
      INCLUDE_DIRECTORIES ${RPC_LIB_INCLUDE_DIRECTORIES}
      SOURCE_PATH ${RPC_LIB_SOURCE_PATH}
      GENERATED_INCLUDES include_paths
      CPP_FILES generated_sources
      CPP_USRV_FILES generated_usrv_sources
  )
  add_library(${NAME} STATIC ${generated_sources} ${generated_usrv_sources})
  target_compile_options(${NAME} PUBLIC -Wno-unused-parameter)
  target_include_directories(${NAME} SYSTEM PUBLIC $<BUILD_INTERFACE:${include_paths}>)

  if(USERVER_CONAN AND NOT CMAKE_PROJECT_NAME STREQUAL userver)
    target_link_libraries(${NAME} PUBLIC userver::grpc)
  elseif (TARGET userver::userver-grpc)
    target_link_libraries(${NAME} PUBLIC userver::userver-grpc)
  else()
    target_link_libraries(${NAME} PUBLIC userver-grpc)
  endif()
endfunction()
