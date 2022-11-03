function(userver_install)

  set(options)
  set(one_value_args)
  set(multi_value_args TARGETS)
  cmake_parse_arguments(USERVER_INSTALL "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  install(TARGETS ${USERVER_INSTALL_TARGETS}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include)

  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "include"
    FILES_MATCHING
    PATTERN "*.hpp"
    )

endfunction()
