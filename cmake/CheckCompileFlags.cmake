function(_filter_compile_flags OPTIONS_VAR DEFINITIONS_VAR)
  set(compile_options_list "${${OPTIONS_VAR}}")
  set(compile_definitions_list "${${DEFINITIONS_VAR}}")

  string(REGEX REPLACE [[\$<\$<COMPILE_LANGUAGE:CXX>:([^>]+)>]] [[\1]]
      compile_options_list "${compile_options_list}")
  string(REGEX REPLACE [[\$<\$<COMPILE_LANGUAGE:CXX>:([^>]+)>]] [[\1]]
      compile_definitions_list "${compile_definitions_list}")

  string(REGEX REPLACE [[-fmacro-prefix-map=[^=]+=]] [[-fmacro-prefix-map=(REDACTED)=]]
      compile_options_list "${compile_options_list}")
  string(REGEX REPLACE [[-fsanitize-blacklist=[^;]+]] [[-fsanitize-blacklist=(REDACTED)]]
      compile_options_list "${compile_options_list}")

  set("${OPTIONS_VAR}" ${compile_options_list} PARENT_SCOPE)
  set("${DEFINITIONS_VAR}" ${compile_definitions_list} PARENT_SCOPE)
endfunction()

function(_get_target_compile_flags TARGET PUBLIC_ONLY OPTIONS_VAR DEFINITIONS_VAR)
  if(PUBLIC_ONLY)
    set(property_prefix "INTERFACE_")
  else()
    set(property_prefix "")
  endif()

  get_target_property(compile_options_list "${TARGET}" "${property_prefix}COMPILE_OPTIONS")
  if(NOT compile_options_list)
    set(compile_options_list)
  endif()

  get_target_property(compile_definitions_list "${TARGET}" "${property_prefix}COMPILE_DEFINITIONS")
  if(NOT compile_definitions_list)
    set(compile_definitions_list)
  endif()

  if(NOT PUBLIC_ONLY)
    get_target_property(target_source_dir "${TARGET}" SOURCE_DIR)
    get_directory_property(dir_compile_definitions_list
        DIRECTORY "${target_source_dir}" COMPILE_DEFINITIONS)
    if(NOT dir_compile_definitions_list)
      set(dir_compile_definitions_list)
    endif()
    list(APPEND compile_definitions_list ${dir_compile_definitions_list})
  endif()

  _filter_compile_flags(compile_options_list compile_definitions_list)

  get_target_property(link_libraries_list "${TARGET}" "${property_prefix}LINK_LIBRARIES")
  foreach(library IN LISTS link_libraries_list)
    if(NOT TARGET "${library}")
      continue()
    endif()
    _get_target_compile_flags("${library}" ON dependency_options dependency_definitions)
    list(APPEND compile_options_list ${dependency_options})
    list(APPEND compile_definitions_list ${dependency_definitions})
  endforeach()

  list(REMOVE_DUPLICATES compile_options_list)
  list(REMOVE_DUPLICATES compile_definitions_list)
  set("${OPTIONS_VAR}" ${compile_options_list} PARENT_SCOPE)
  set("${DEFINITIONS_VAR}" ${compile_definitions_list} PARENT_SCOPE)
endfunction()

function(_check_compile_flags TARGET)
  set(USERVER_IMPL_TEST_COMPILE_FLAGS OFF CACHE INTERNAL
      "Whether to check that compile options and definitions equal the given values")

  if(NOT USERVER_IMPL_TEST_COMPILE_FLAGS)
    return()
  endif()

  _get_target_compile_flags("${TARGET}" OFF compile_options_list compile_definitions_list)

  message(STATUS "${TARGET} compile options: ${compile_options_list}")
  message(STATUS "${TARGET} compile definitions: ${compile_definitions_list}")

  set(given_compile_options_var "USERVER_IMPL_TEST_OPTIONS_${TARGET}")
  set(given_compile_definitions_var "USERVER_IMPL_TEST_DEFINITIONS_${TARGET}")
  set("${given_compile_options_var}" MISSING CACHE INTERNAL
      "Expected COMPILE_OPTIONS for ${TARGET} target")
  set("${given_compile_definitions_var}" MISSING CACHE INTERNAL
      "Expected COMPILE_DEFINITIONS for ${TARGET} target")
  set(given_compile_options "${${given_compile_options_var}}")
  set(given_compile_definitions "${${given_compile_definitions_var}}")

  set(is_error OFF)

  if(NOT given_compile_options STREQUAL compile_options_list)
    set(is_error ON)
    message(SEND_ERROR
        "Actual COMPILE_OPTIONS for ${TARGET} target are not equal "
        "to the given ${given_compile_options_var} cache variable\n"
        "actual: ${compile_options_list}\n"
        "given: ${given_compile_options}")
  endif()

  if(NOT given_compile_definitions STREQUAL compile_definitions_list)
    set(is_error ON)
    message(SEND_ERROR
        "Actual COMPILE_DEFINITIONS for ${TARGET} target are not equal "
        "to the given ${given_compile_definitions_var} cache variable\n"
        "actual: ${compile_definitions_list}\n"
        "given: ${given_compile_definitions}")
  endif()

  if(NOT is_error)
    message(STATUS "Compile flags check success for ${TARGET} target")
  endif()
endfunction()
