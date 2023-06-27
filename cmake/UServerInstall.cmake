function(userver_install_library)

  set(options)
  set(one_value_args)
  set(multi_value_args TARGETS INCLUDE_DIR)
  cmake_parse_arguments(USERVER_INSTALL "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
  
  install(TARGETS ${USERVER_INSTALL_TARGETS} EXPORT userver-targets
    PUBLIC_HEADER DESTINATION include)

  if (NOT DEFINED USERVER_INSTALL_INCLUDE_DIR)
    set(USERVER_INSTALL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/")
  endif()

  install(DIRECTORY "${USERVER_INSTALL_INCLUDE_DIR}/"
    DESTINATION "include"
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.ipp"
    PATTERN "*.h"
    PATTERN "*.inc"
    )

endfunction()

function(userver_export)
  set(options)
  set(one_value_args)
  set(multi_value_args TARGETS)
  cmake_parse_arguments(USERVER_EXPORT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  install(TARGETS ${USERVER_EXPORT_TARGETS} EXPORT userver-targets
    PUBLIC_HEADER DESTINATION include)

endfunction()
