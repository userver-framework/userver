include(CTest)

set(TESTSUITE_ROOT_DIR ${CMAKE_SOURCE_DIR}/submodules/testsuite)
set(TESTSUITE_PYTHONPATH
  ${TESTSUITE_ROOT_DIR}:${CMAKE_SOURCE_DIR}/testsuite:$ENV{PYTHONPATH})
set(TESTSUITE_PYTHON_BINARY python2.7)
set(TESTSUITE_PYTHON
  PYTHONPATH=${TESTSUITE_PYTHONPATH} ${TESTSUITE_PYTHON_BINARY})
set(TESTSUITE_GEN_FASTCGI
  ${TESTSUITE_PYTHON} ${CMAKE_SOURCE_DIR}/testsuite/scripts/genfastcgi.py)
set(TESTSUITE_DB_SETTINGS_YAML ${CMAKE_SOURCE_DIR}/db_settings.yaml)

unset(TESTSUITE_PYTEST_DIRS CACHE)

function(testsuite_generate_userver_descriptor NAME BINARY CONFIG)
  configure_file(
    ${CMAKE_SOURCE_DIR}/testsuite/scripts/service.in
    ${CMAKE_BINARY_DIR}/testsuite/services/${NAME}.service
    @ONLY)
endfunction()

function(testsuite_userver_project PROJECT)
  set(PROJECT_SERVICE_CONFIG_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/testsuite/configs/service.json)
  set(PROJECT_SECDIST_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/testsuite/configs/secdist.json)

  testsuite_generate_userver_descriptor(
    ${PROJECT}
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT}
    ${PROJECT_SERVICE_CONFIG_PATH})

  add_custom_command(
    OUTPUT ${PROJECT_SECDIST_PATH}
    COMMENT "Creating testsuite secdist config for ${PROJECT}"
    COMMAND
    mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/testsuite/configs
    COMMAND
    ${TESTSUITE_PYTHON} ${CMAKE_SOURCE_DIR}/testsuite/scripts/gensecdist.py
    --testsuite-build-dir ${CMAKE_CURRENT_BINARY_DIR}
    --testsuite-dir ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/secdist_dev.json
    ${PROJECT_SECDIST_PATH}
    ${TESTSUITE_DB_SETTINGS_YAML}
    DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/secdist_dev.json
    ${CMAKE_SOURCE_DIR}/testsuite/scripts/gensecdist.py
    ${TESTSUITE_DB_SETTINGS_YAML})

  add_custom_command(
    OUTPUT ${PROJECT_SERVICE_CONFIG_PATH}
    COMMENT "Creating testsuite service config file ${PROJECT}"
    COMMAND
    mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/testsuite/configs
    COMMAND
    ${TESTSUITE_PYTHON}
    ${CMAKE_SOURCE_DIR}/testsuite/scripts/gen-userver-config.py
    --userver-config=${CMAKE_CURRENT_SOURCE_DIR}/configs/config.json
    --userver-fallbacks-config=${CMAKE_CURRENT_SOURCE_DIR}/configs/taxi_config_fallback.production.json
    --userver-config-vars=${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testsuite.json
    --secdist=${PROJECT_SECDIST_PATH}
    --output=${PROJECT_SERVICE_CONFIG_PATH}
    DEPENDS
    ${MANUAL_SECDIST_JSON}
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/config.json
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/config_vars.testsuite.json
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/secdist_dev.json
    ${CMAKE_CURRENT_SOURCE_DIR}/configs/taxi_config_fallback.production.json
    ${CMAKE_SOURCE_DIR}/testsuite/scripts/gen-userver-config.py)

  add_custom_target(${PROJECT}-testsuite-configs
    DEPENDS
    ${PROJECT_SERVICE_CONFIG_PATH}
    ${PROJECT_SECDIST_PATH})
  add_dependencies(testsuite-configs ${PROJECT}-testsuite-configs)

  set(TESTSUITE_PYTEST_DIRS ${TESTSUITE_PYTEST_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/testsuite
    CACHE STRING "Directories with all necessary tests." FORCE)

endfunction()

add_custom_target(testsuite-configs ALL
  COMMAND mkdir -p ${CMAKE_BINARY_DIR}/test-results)
