include_guard(GLOBAL)

set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

function(_userver_install_targets)
  set(oneValueArgs COMPONENT)
  set(multiValueArgs TARGETS)
  cmake_parse_arguments(
      ARG "${option}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}"
  )
  if(NOT ARG_COMPONENT)
    message(FATAL_ERROR "No COMPONENT for install")
  endif()
  if(NOT ARG_TARGETS)
    message(FATAL_ERROR "No TARGETS given for install")
  endif()

  foreach(target IN LISTS ARG_TARGETS)
    if(NOT TARGET ${target})
      message(FATAL_ERROR "${target} is not a target. You should use only targets")
    endif()
    if(target MATCHES "^userver-.*")
      string(REGEX REPLACE "^userver-" "" target_without_userver "${target}")
      add_library("userver::${target_without_userver}" ALIAS "${target}")
      set_target_properties("${target}" PROPERTIES
          EXPORT_NAME "${target_without_userver}"
      )
    endif()
  endforeach()

  if(NOT USERVER_INSTALL)
    return()
  endif()

  install(
      TARGETS ${ARG_TARGETS}
      EXPORT userver-targets
      CONFIGURATIONS RELEASE
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${ARG_COMPONENT}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(
      TARGETS ${ARG_TARGETS}
      EXPORT userver-targets_d
      CONFIGURATIONS DEBUG
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${ARG_COMPONENT}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${ARG_COMPONENT}
      INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endfunction()

function(_userver_export_targets)
  if(NOT USERVER_INSTALL)
    return()
  endif()
  install(EXPORT userver-targets
          FILE userver-targets.cmake
          CONFIGURATIONS RELEASE
          NAMESPACE userver::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/release
  )
  install(EXPORT userver-targets_d
          FILE userver-targets_d.cmake
          CONFIGURATIONS DEBUG
          NAMESPACE userver::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/debug
  )
endfunction()

function(_userver_directory_install)
  if(NOT USERVER_INSTALL)
    return()
  endif()
  set(oneValueArgs COMPONENT DESTINATION PATTERN)
  set(multiValueArgs FILES DIRECTORY PROGRAMS)
  cmake_parse_arguments(
    ARG "${option}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}"
  )
  if(NOT ARG_COMPONENT)
    message(FATAL_ERROR "No COMPONENT for install")
  endif()
  if(NOT ARG_DESTINATION)
    message(FATAL_ERROR "No DESTINATION for install")
  endif()
  if(NOT ARG_FILES AND NOT ARG_DIRECTORY AND NOT ARG_PROGRAMS)
    message(FATAL_ERROR "No FILES or DIRECTORY or PROGRAMS provided to install")
  endif()
  if(ARG_PROGRAMS)
    install(PROGRAMS ${ARG_PROGRAMS} DESTINATION ${ARG_DESTINATION} COMPONENT ${ARG_COMPONENT})
  endif()
  if(ARG_FILES)
    install(FILES ${ARG_FILES} DESTINATION ${ARG_DESTINATION} COMPONENT ${ARG_COMPONENT})
  endif()
  if(ARG_DIRECTORY)
    if (ARG_PATTERN)
      install(DIRECTORY ${ARG_DIRECTORY} DESTINATION ${ARG_DESTINATION} COMPONENT ${ARG_COMPONENT} USE_SOURCE_PERMISSIONS FILES_MATCHING PATTERN ${ARG_PATTERN})
    else()
      install(DIRECTORY ${ARG_DIRECTORY} DESTINATION ${ARG_DESTINATION} COMPONENT ${ARG_COMPONENT} USE_SOURCE_PERMISSIONS)
    endif()
  endif()
endfunction()

function(_userver_make_install_config)
  if(NOT USERVER_INSTALL)
    return()
  endif()

  configure_package_config_file(
      "${USERVER_ROOT_DIR}/cmake/install/Config.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/userverConfig.cmake"
      INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
  )

  write_basic_package_version_file(
      "${CMAKE_CURRENT_BINARY_DIR}/userverConfigVersion.cmake"
      VERSION "${USERVER_VERSION}" COMPATIBILITY SameMajorVersion
  )

  _userver_directory_install(COMPONENT universal FILES
      "${CMAKE_CURRENT_BINARY_DIR}/userverConfig.cmake"
      "${CMAKE_CURRENT_BINARY_DIR}/userverConfigVersion.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
  )
endfunction()
