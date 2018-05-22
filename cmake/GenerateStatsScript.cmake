function(generate_stats_script PROJECT NAME HOSTNAME FILENAME)
  set(STATS_SCRIPT_TEMPLATE ${CMAKE_SOURCE_DIR}/scripts/templates/taxi-stats.tpl.py)
  set(STATS_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.py)

  add_custom_command(OUTPUT ${STATS_SCRIPT}
    COMMENT "Generating stats script ${FILENAME}"
    COMMAND sed "s#%NAME%#${NAME}#g\;s#%HOSTNAME%#${HOSTNAME}#g"
      ${STATS_SCRIPT_TEMPLATE} > ${STATS_SCRIPT}
    DEPENDS ${STATS_SCRIPT_TEMPLATE})
  add_custom_target(taxi-stats-${FILENAME} DEPENDS ${STATS_SCRIPT})
  add_dependencies(${PROJECT} taxi-stats-${FILENAME})

  include(GNUInstallDirs)
  install(FILES ${STATS_SCRIPT} DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction(generate_stats_script)
