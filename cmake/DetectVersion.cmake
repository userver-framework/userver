function(deb_version version_output_var debpackage)
  find_program(DPKG_QUERY_BIN dpkg-query)
  if (NOT DPKG_QUERY_BIN)
    message(FATAL_ERROR "Failed to find dpkg-query")
  endif()

  execute_process(
    COMMAND ${DPKG_QUERY_BIN} --showformat=\${Version} --show ${debpackage}
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (version_result EQUAL 0)
    set(${version_output_var} ${version_output} PARENT_SCOPE)
    message(STATUS "Installed version ${debpackage}: ${version_output}")
  else()
    message(STATUS "${version_error}")
  endif()
endfunction()


function(brew_version version_output_var brewpackage)
  find_program(BREW_BIN brew)
  if (NOT BREW_BIN)
    message(FATAL_ERROR "Failed to find brew")
  endif()

  execute_process(
    COMMAND ${BREW_BIN} list --versions ${brewpackage}
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (version_result EQUAL 0)
    if (version_output MATCHES "^(.*) (.*)$")
      set(${version_output_var} ${CMAKE_MATCH_2} PARENT_SCOPE)
      message(STATUS "Installed version ${brewpackage}: ${Grpc_VERSION}")
    else()
      set(${version_output_var} "NOT_FOUND")
    endif()
  else()
    message(FATAL_ERROR "Failed execute brew: ${version_error}")
  endif()
endfunction()
