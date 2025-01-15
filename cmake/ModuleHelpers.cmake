include_guard(GLOBAL)

cmake_policy(SET CMP0054 NEW)

macro(_userver_module_begin)
  set(options)
  set(oneValueArgs
      # Target name, also used for package name by default
      NAME
      VERSION
  )
  set(multiValueArgs
      DEBIAN_NAMES
      FORMULA_NAMES
      RPM_NAMES
      PACMAN_NAMES
      PKG_NAMES
      # For version detection of manually installed packages and unknown
      # package managers.
      PKG_CONFIG_NAMES
  )

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  set(name "${ARG_NAME}")

  if(ARG_VERSION)
    if(NOT ${name}_FIND_VERSION OR "${${name}_FIND_VERSION}" VERSION_LESS "${ARG_VERSION}")
      set("${name}_FIND_VERSION" "${ARG_VERSION}")
    endif()
  endif()

  if(NOT USERVER_CHECK_PACKAGE_VERSIONS)
    unset("${name}_FIND_VERSION")
  endif()

  if(TARGET "${name}")
    if(NOT ${name}_FIND_VERSION)
      set("${name}_FOUND" ON)
      set("${name}_SKIP_USERVER_FIND" ON)
      return()
    endif()

    if(${name}_VERSION)
      if(${name}_FIND_VERSION VERSION_LESS_EQUAL ${name}_VERSION)
        set("${name}_FOUND" ON)
        set("${name}_SKIP_USERVER_FIND" ON)
        return()
      else()
        message(FATAL_ERROR
            "Already using version ${${name}_VERSION}"
            "of ${name} when version ${${name}_FIND_VERSION} "
            "was requested."
        )
      endif()
    endif()
  endif()

  set(FULL_ERROR_MESSAGE "Could not find `${name}` package.")
  if(ARG_DEBIAN_NAMES)
    list(JOIN ARG_DEBIAN_NAMES " " package_names_joined)
    string(APPEND FULL_ERROR_MESSAGE "\\n\\tDebian: sudo apt update && sudo apt install ${package_names_joined}")
  endif()
  if(ARG_FORMULA_NAMES)
    list(JOIN ARG_FORMULA_NAMES " " package_names_joined)
    string(APPEND FULL_ERROR_MESSAGE "\\n\\tMacOS: brew install ${package_names_joined}")
  endif()
  if(ARG_RPM_NAMES)
    list(JOIN ARG_RPM_NAMES " " package_names_joined)
    string(APPEND FULL_ERROR_MESSAGE "\\n\\tFedora: sudo dnf install ${package_names_joined}")
  endif()
  if(ARG_PACMAN_NAMES)
    list(JOIN ARG_PACMAN_NAMES " " package_names_joined)
    string(APPEND FULL_ERROR_MESSAGE "\\n\\tArchLinux: sudo pacman -S ${package_names_joined}")
  endif()
  if(ARG_PKG_NAMES)
    list(JOIN ARG_PKG_NAMES " " package_names_joined)
    string(APPEND FULL_ERROR_MESSAGE "\\n\\tFreeBSD: pkg install ${package_names_joined}")
  endif()
  string(APPEND FULL_ERROR_MESSAGE "\\n")

  set("${ARG_NAME}_LIBRARIES")
  set("${ARG_NAME}_INCLUDE_DIRS")
  set("${ARG_NAME}_EXECUTABLE")
endmacro()

macro(_userver_module_find_part)
  # Also uses ARGs left over from _userver_find_module_begin

  set(options)
  set(oneValueArgs
      PART_TYPE
  )
  set(multiValueArgs
      NAMES
      PATHS
      PATH_SUFFIXES
  )

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  if(${ARG_NAME}_SKIP_USERVER_FIND)
    return()
  endif()

  if("${ARG_PART_TYPE}" STREQUAL "library")
    set(variable "${ARG_NAME}_LIBRARIES")
  elseif("${ARG_PART_TYPE}" STREQUAL "path")
    set(variable "${ARG_NAME}_INCLUDE_DIRS")
  elseif("${ARG_PART_TYPE}" STREQUAL "program")
    set(variable "${ARG_NAME}_EXECUTABLE")
  else()
    message(FATAL_ERROR "Invalid PART_TYPE")
  endif()

  list(JOIN ARG_NAMES _ names_joined)
  string(REPLACE / _ names_joined "${names_joined}")
  string(REPLACE . _ names_joined "${names_joined}")
  set(mangled_name "${variable}_${names_joined}")

  set(find_command_args
      "${mangled_name}"
      NAMES ${ARG_NAMES}
      PATH_SUFFIXES ${ARG_PATH_SUFFIXES}
      PATHS ${ARG_PATHS}
  )
  message(DEBUG "STATUS FIND(${ARG_NAMES}) ARGS: ${find_command_args}")
  if("${ARG_PART_TYPE}" STREQUAL "library")
    find_library(${find_command_args})
  elseif("${ARG_PART_TYPE}" STREQUAL "path")
    find_path(${find_command_args})
  elseif("${ARG_PART_TYPE}" STREQUAL "program")
    find_program(${find_command_args})
  else()
    message(FATAL_ERROR "Invalid PART_TYPE")
  endif()
  list(APPEND "${variable}" "${${mangled_name}}")

  unset(variable)
  unset(names_joined)
  unset(mangled_name)
  unset(find_command_args)
endmacro()

macro(_userver_module_find_library)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs
      NAMES
      PATHS
      PATH_SUFFIXES
  )

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  _userver_module_find_part(
      PART_TYPE library
      NAMES ${ARG_NAMES}
      PATHS ${ARG_PATHS}
      PATH_SUFFIXES ${ARG_PATH_SUFFIXES}
  )
endmacro()

macro(_userver_module_find_include)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs
      NAMES
      PATHS
      PATH_SUFFIXES
  )

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  _userver_module_find_part(
      PART_TYPE path
      NAMES ${ARG_NAMES}
      PATHS ${ARG_PATHS}
      PATH_SUFFIXES ${ARG_PATH_SUFFIXES}
  )
endmacro()

macro(_userver_module_find_program)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs
      NAMES
      PATHS
      PATH_SUFFIXES
  )

  cmake_parse_arguments(
      ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

  _userver_module_find_part(
      PART_TYPE program
      NAMES ${ARG_NAMES}
      PATHS ${ARG_PATHS}
      PATH_SUFFIXES ${ARG_PATH_SUFFIXES}
  )
endmacro()

macro(_userver_module_end)
  # Uses ARGs left over from _userver_find_module_begin

  if(${ARG_NAME}_SKIP_USERVER_FIND)
    return()
  endif()

  get_property(USERVER_CMAKE_DIR GLOBAL PROPERTY userver_cmake_dir)
  if(NOT USERVER_CMAKE_DIR)
    message(
        FATAL_ERROR
        "userver_setup_environment() should be run before trying to use any Find* from userver"
    )
  endif()

  include(FindPackageHandleStandardArgs)
  include("${USERVER_CMAKE_DIR}/DetectVersion.cmake")

  set(name "${ARG_NAME}")
  set(current_package_name "${ARG_NAME}")
  set(libraries_variable "${ARG_NAME}_LIBRARIES")
  set(includes_variable "${ARG_NAME}_INCLUDE_DIRS")
  set(programs_variable "${ARG_NAME}_EXECUTABLE")

  if(${name}_VERSION)
    set("${current_package_name}_VERSION" "${${name}_VERSION}")
  endif()

  # If it is possible detect the version as the dependent CMake files may rely on it
  if(NOT ${current_package_name}_VERSION)
    if(ARG_DEBIAN_NAMES AND UNIX AND NOT APPLE)
      list(GET ARG_DEBIAN_NAMES 0 first_package_name)
      deb_version("${current_package_name}_VERSION" "${first_package_name}")
    endif()

    if(ARG_RPM_NAMES AND UNIX AND NOT APPLE)
      list(GET ARG_RPM_NAMES 0 first_package_name)
      rpm_version("${current_package_name}_VERSION" "${first_package_name}")
    endif()

    if(ARG_PACMAN_NAMES AND UNIX AND NOT APPLE)
      list(GET ARG_PACMAN_NAMES 0 first_package_name)
      pacman_version("${current_package_name}_VERSION" "${first_package_name}")
    endif()

    if(ARG_PKG_NAMES AND UNIX AND NOT APPLE)
      list(GET ARG_PKG_NAMES 0 first_package_name)
      pkg_version("${current_package_name}_VERSION" "${first_package_name}")
    endif()

    if(ARG_FORMULA_NAMES AND APPLE)
      list(GET ARG_FORMULA_NAMES 0 first_package_name)
      brew_version("${current_package_name}_VERSION" "${first_package_name}")
    endif()

    if(ARG_PKG_CONFIG_NAMES)
      list(GET ARG_PKG_CONFIG_NAMES 0 first_package_name)
      pkg_config_version("${current_package_name}_VERSION" "${first_package_name}")
    endif()
  endif()

  set(required_vars)
  # Important to check this way, because NOTFOUND still means that this
  # kind of dependencies exist.
  if(NOT "${${libraries_variable}}" STREQUAL "")
    list(APPEND required_vars "${libraries_variable}")
  endif()
  if(NOT "${${includes_variable}}" STREQUAL "")
    list(APPEND required_vars "${includes_variable}")
  endif()
  if(NOT "${${programs_variable}}" STREQUAL "")
    list(APPEND required_vars "${programs_variable}")
  endif()
  if(required_vars)
    find_package_handle_standard_args(
        "${current_package_name}"
        REQUIRED_VARS
        ${required_vars}
        FAIL_MESSAGE
        "${FULL_ERROR_MESSAGE}"
    )
    mark_as_advanced(${required_vars})
  else()
    # Forward to another CMake module, add nice error messages if missing.
    set(wrapped_package_name "${current_package_name}")
    set(find_command_args "${wrapped_package_name}")
    if(ARG_VERSION)
      list(APPEND find_command_args "${ARG_VERSION}")
    endif()
    find_package(${find_command_args})
    set("${name}_FOUND" "${${wrapped_package_name}_FOUND}")
  endif()

  if(NOT ${name}_FOUND)
    if(${name}_FIND_REQUIRED)
      message(FATAL_ERROR "${FULL_ERROR_MESSAGE}. Required version is at least ${${name}_FIND_VERSION}")
    endif()
    return()
  endif()

  if(${name}_FIND_VERSION)
    if("${${current_package_name}_VERSION}" VERSION_LESS "${${name}_FIND_VERSION}")
      message(STATUS
          "Version of ${name} is ${${current_package_name}_VERSION}. "
          "Required version is at least ${${name}_FIND_VERSION}. "
          "Ignoring found ${name}. "
          "Note: Set -DUSERVER_CHECK_PACKAGE_VERSIONS=0 to skip package "
          "version checks if the package is fine."
      )
      set("${name}_FOUND" OFF)
      return()
    endif()
  endif()

  if((NOT "${${libraries_variable}}" STREQUAL "")
      OR (NOT "${${includes_variable}}" STREQUAL ""))
    if(NOT TARGET "${name}")
      add_library("${name}" INTERFACE IMPORTED GLOBAL)

      if(NOT "${${includes_variable}}" STREQUAL "")
        target_include_directories("${name}" INTERFACE ${${includes_variable}})
        message(STATUS "${name} include directories: ${${includes_variable}}")
      endif()

      if(NOT "${${libraries_variable}}" STREQUAL "")
        target_link_libraries("${name}" INTERFACE ${${libraries_variable}})
        message(STATUS "${name} libraries: ${${libraries_variable}}")
      endif()
    endif()

    if(${current_package_name}_VERSION)
      set("${name}_VERSION" "${${current_package_name}_VERSION}" CACHE STRING "Version of the ${name}")
    endif()
  endif()
endmacro()

function(_userver_macos_set_default_dir variable command_args)
  set(default_value "")
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin" AND NOT DEFINED ${variable})
    execute_process(
        COMMAND ${command_args}
        OUTPUT_VARIABLE output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(default_value "${output}")
  endif()
  set("${variable}" "${default_value}" CACHE PATH "")
endfunction()

function(_userver_print_features_list)
  get_cmake_property(variable_names CACHE_VARIABLES)
  list(SORT variable_names)
  message(STATUS "userver features enabled:")
  foreach(variable ${variable_names})
    string(SUBSTRING ${variable} 0 15 variable_prefix)
    if(NOT ${variable_prefix} STREQUAL USERVER_FEATURE)
      continue()
    endif()
    if(${variable})
      message(STATUS " * ${variable}")
    endif()
  endforeach()

  message(STATUS "userver features disabled:")
  foreach(variable ${variable_names})
    string(SUBSTRING ${variable} 0 15 variable_prefix)
    if(NOT ${variable_prefix} STREQUAL USERVER_FEATURE)
      continue()
    endif()
    if(NOT ${variable})
      message(STATUS " * ${variable}")
    endif()
  endforeach()
endfunction()
