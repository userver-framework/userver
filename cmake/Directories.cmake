include_guard(GLOBAL)

if(CMAKE_VERSION VERSION_LESS "3.17")
  set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")
endif()

function(_get_userver_cmake_dir RESULT_VAR)
  if(CMAKE_VERSION VERSION_LESS "3.17")
    get_property(userver_cmake_dir GLOBAL PROPERTY userver_cmake_dir)
    set("${RESULT_VAR}" "${userver_cmake_dir}" PARENT_SCOPE)
  else()
    set("${RESULT_VAR}" "${CMAKE_CURRENT_FUNCTION_LIST_DIR}" PARENT_SCOPE)
  endif()
endfunction()

function(_is_userver_imported RESULT_VAR)
  if(TARGET userver-universal)
    set(some_userver_target userver-universal)
  elseif(TARGET userver::universal)
    set(some_userver_target userver::universal)
  else()
    message(FATAL_ERROR "Call add_subdirectory(userver) before the current function")
  endif()

  get_target_property(userver_is_imported "${some_userver_target}" IMPORTED)
  set("${RESULT_VAR}" "${userver_is_imported}" PARENT_SCOPE)
endfunction()

# Returns false-y result if userver is imported.
function(_get_userver_binary_dir RESULT_VAR)
  if(DEFINED USERVER_NOT_INCLUDED_AS_SUBDIR AND
      NOT DEFINED CACHE{USERVER_NOT_INCLUDED_AS_SUBDIR})
    # Called from main userver CMakeLists.txt
  else()
    _is_userver_imported(is_userver_imported)
    if(userver_is_imported)
      set("${RESULT_VAR}" "" PARENT_SCOPE)
      return()
    endif()
  endif()

  _get_userver_cmake_dir(userver_cmake_dir)
  # Expect that the userver directory structure is intact
  get_filename_component(userver_source_dir "${userver_cmake_dir}" DIRECTORY)
  get_directory_property(userver_binary_dir DIRECTORY "${userver_source_dir}" BINARY_DIR)
  if(NOT userver_binary_dir)
    message(FATAL_ERROR "unexpected userver directory structure")
  endif()

  set("${RESULT_VAR}" "${userver_binary_dir}" PARENT_SCOPE)
endfunction()
