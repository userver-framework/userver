include_guard(GLOBAL)

function(userver_embed_file TARGET)
  set(OPTIONS)
  set(ONE_VALUE_ARGS NAME FILEPATH HPP_FILENAME)
  set(MULTI_VALUE_ARGS SQL_FILES)
  cmake_parse_arguments(
      ARG "${OPTIONS}" "${ONE_VALUE_ARGS}" "${MULTI_VALUE_ARGS}" ${ARGN}
  )
  if(NOT ARG_HPP_FILENAME)
    # new cmake: cmake_path(GET ARG_FILEPATH FILENAME ARG_HPP_FILENAME)
    string(REGEX REPLACE ".*/" "" ARG_HPP_FILENAME "${ARG_FILEPATH}")
  endif()

  string(SUBSTRING "${ARG_FILEPATH}" 0 1 START)
  if(NOT START STREQUAL /)
    set(ARG_FILEPATH "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_FILEPATH}")
  endif()

  set(CONFIG_HPP ${CMAKE_CURRENT_BINARY_DIR}/embedded/include/generated/${ARG_HPP_FILENAME}.hpp)
  add_custom_command(
    OUTPUT
    ${CONFIG_HPP}
    DEPENDS
        ${USERVER_ROOT_DIR}/cmake/embedded_config.cmake
    COMMAND
        ${CMAKE_COMMAND}
	    -DUSERVER_ROOT_DIR=${USERVER_ROOT_DIR}
	    -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
	    -DFILEPATH=${ARG_FILEPATH}
	    -DOUTPUT=${CONFIG_HPP}
	    -DNAME=${ARG_NAME}
	    -P ${USERVER_ROOT_DIR}/cmake/embedded_config.cmake
  )
  add_library(${TARGET} STATIC ${CONFIG_HPP})
  target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/embedded/include)
  target_link_libraries(${TARGET} PUBLIC userver::universal)
endfunction()
