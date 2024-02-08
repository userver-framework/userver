include_guard(GLOBAL)

set_property(GLOBAL PROPERTY userver_cmake_dir "${CMAKE_CURRENT_LIST_DIR}")

function(_userver_install_targets)
  set(multiValueArgs TARGETS)
  cmake_parse_arguments(
    ARG "" "" "${multiValueArgs}" "${ARGN}"
  )
  if(NOT ARG_TARGETS)
    message(FATAL_ERROR "No TARGETS given for install")
    return()
  endif()
  foreach(ITEM_TARGET IN LISTS ARG_TARGETS)
    if(NOT TARGET ${ITEM_TARGET})
      message(FATAL_ERROR "${ITEM_TARGET} is not target. You should use only targets")
      return()
    endif()
  endforeach()
  if(NOT USERVER_INSTALL)
    return()
  endif()
  install(TARGETS ${ARG_TARGETS}
          EXPORT UserverTargets
          CONFIGURATIONS RELEASE
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
  install(TARGETS ${ARG_TARGETS}
          EXPORT UserverTargetsD
          CONFIGURATIONS DEBUG
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endfunction()

function(_userver_export_targets)
  if(NOT USERVER_INSTALL)
    return()
  endif()
  install(EXPORT UserverTargets
          FILE UserverTargets.cmake
          CONFIGURATIONS RELEASE
          NAMESPACE userver::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/release
  )
  install(EXPORT UserverTargetsD
          FILE UserverTargetsD.cmake
          CONFIGURATIONS DEBUG
          NAMESPACE userver::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/userver/debug
  )
endfunction()
