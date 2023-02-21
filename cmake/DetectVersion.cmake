macro(userver_version_strip_trash VERSION_OUTPUT_VAR)
  STRING(REGEX REPLACE "^1:" "" ${VERSION_OUTPUT_VAR} "${${VERSION_OUTPUT_VAR}}")
endmacro()

function(rpm_version version_output_var rpmpackage)
  if (${version_output_var})
    return()
  endif()

  if (NOT RPM_QUERY_BIN AND NOT RPM_QUERY_BIN STREQUAL "RPM_QUERY_BIN-NOTFOUND")
    find_program(RPM_QUERY_BIN rpm)
    if (NOT RPM_QUERY_BIN)
      message(STATUS "Failed to find 'rpm'")
    endif()
  endif()

  if (NOT RPM_QUERY_BIN)
    return()
  endif()

  execute_process(
    COMMAND ${RPM_QUERY_BIN} -q --qf "%{VERSION}" ${rpmpackage}
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (version_result EQUAL 0)
    userver_version_strip_trash(version_output)
    set(${version_output_var} ${version_output} PARENT_SCOPE)
    message(STATUS "rpm version of ${rpmpackage}: ${version_output}")
  else()
    message(STATUS "${version_error}")
  endif()
endfunction()

function(deb_version version_output_var debpackage)
  if (${version_output_var})
    return()
  endif()

  if (NOT DPKG_QUERY_BIN AND NOT DPKG_QUERY_BIN STREQUAL "DPKG_QUERY_BIN-NOTFOUND")
    find_program(DPKG_QUERY_BIN dpkg-query)
    if (NOT DPKG_QUERY_BIN)
      message(STATUS "Failed to find 'dpkg-query'")
    endif()
  endif()

  if (NOT DPKG_QUERY_BIN)
    return()
  endif()

  execute_process(
    COMMAND ${DPKG_QUERY_BIN} --showformat=\${Version} --show ${debpackage}
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (version_result EQUAL 0)
    userver_version_strip_trash(version_output)
    set(${version_output_var} ${version_output} PARENT_SCOPE)
    message(STATUS "dpkg-query version of ${debpackage}: ${version_output}")
  else()
    message(STATUS "${version_error}")
  endif()
endfunction()

function(pacman_version version_output_var pacmanpackage)
  if (${version_output_var})
    return()
  endif()

  if (NOT PACMAN_BIN AND NOT PACMAN_BIN STREQUAL "PACMAN_BIN-NOTFOUND")
    find_program(PACMAN_BIN pacman)
    if (NOT PACMAN_BIN)
      message(STATUS "Failed to find 'pacman'")
    endif()
  endif()

  if (NOT PACMAN_BIN)
    return()
  endif()

  execute_process(
    COMMAND ${PACMAN_BIN} -Q ${pacmanpackage}
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (version_result EQUAL 0)
    if (version_output MATCHES "^(.*) (.*)-.*$")
      set(${version_output_var} ${CMAKE_MATCH_2} PARENT_SCOPE)
      message(STATUS "pacman version of ${pacmanpackage}: ${CMAKE_MATCH_2}")
    else()
      set(${version_output_var} "NOT_FOUND")
    endif()
  else()
    set(${version_output_var} "NOT_FOUND")
    message(ERROR "Failed execute pacman: ${version_error}")
  endif()
endfunction()

function(pkg_config_version version_output_var pkg_config_name)
  # pkg-config has issues on MacOS https://github.com/Homebrew/brew/issues/5068
  if (APPLE)
    return()
  endif()

  if (${version_output_var})
    return()
  endif()

  if (NOT PKG_CONFIG_FOUND AND NOT PKG_CONFIG_VERSION_STRING STREQUAL "PKG_CONFIG_VERSION_STRING-NOTFOUND")
    find_package(PkgConfig)
    if (NOT PKG_CONFIG_FOUND)
      message(STATUS "Failed to find 'pkg-config'")
    endif()
  endif()

  if (NOT PKG_CONFIG_FOUND)
    return()
  endif()

  pkg_check_modules(PKG_CHECK ${pkg_config_name})

  if (PKG_CHECK_FOUND)
    if (PKG_CHECK_VERSION)
      userver_version_strip_trash(PKG_CHECK_VERSION)
      set(${version_output_var} ${PKG_CHECK_VERSION} PARENT_SCOPE)
      message(STATUS "pkg-config version of ${pkg_config_name}: ${PKG_CHECK_VERSION}")
    else()
      set(${version_output_var} "NOT_FOUND")
    endif()
  else()
    set(${version_output_var} "NOT_FOUND")
    message(ERROR "Failed to find ${pkg_config_name} via pkg-config")
  endif()
endfunction()


function(brew_version version_output_var brewpackage)
  if (${version_output_var})
    return()
  endif()

  find_program(BREW_BIN brew)
  if (NOT BREW_BIN)
    message(FATAL_ERROR "Failed to find 'brew'")
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
      message(STATUS "brew version of ${brewpackage}: ${CMAKE_MATCH_2}")
    else()
      set(${version_output_var} "NOT_FOUND")
    endif()
  else()
    if (version_error STREQUAL "")
      message(STATUS "Failed to find brew package version for '${brewpackage}'")
      set(${version_output_var} "NOT_FOUND")
    else()
      message(FATAL_ERROR "Failed execute brew: ${version_error}")
    endif()
  endif()
endfunction()


function(pkg_version version_output_var pkgpackage)
  if (${version_output_var})
    return()
  endif()

  if (NOT PKG_BIN AND NOT PKG_BIN STREQUAL "PKG_BIN-NOTFOUND")
    find_program(PKG_BIN pkg)
    if (NOT PKG_BIN)
      message(STATUS "Failed to find 'pkg'")
    endif()
  endif()

  if (NOT PKG_BIN)
    return()
  endif()

  execute_process(
    COMMAND ${PKG_BIN} version -n ${pkgpackage}
    OUTPUT_VARIABLE version_output
    ERROR_VARIABLE version_error
    RESULT_VARIABLE version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (version_result EQUAL 0)
    if (version_output MATCHES "^.*-([0-9.]*)")
      set(${version_output_var} ${CMAKE_MATCH_1} PARENT_SCOPE)
      message(STATUS "pkg version of ${pkgpackage}: ${CMAKE_MATCH_1}")
    else()
      set(${version_output_var} "NOT_FOUND")
    endif()
  else()
    set(${version_output_var} "NOT_FOUND")
    message(ERROR "Failed execute pkg: ${version_error}")
  endif()
endfunction()
