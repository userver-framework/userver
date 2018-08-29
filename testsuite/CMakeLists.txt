add_test(
  NAME testsuite
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/runtests
  -vv -x
  --junitxml=${CMAKE_BINARY_DIR}/test-results/junit-testsuite.xml
  -p no:name_of_plugin -p no:pytest-teamcity
  $<JOIN:$<TARGET_PROPERTY:testsuite-configs,TESTSUITE_PYTEST_DIRS>," ">
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(testsuite PROPERTIES TIMEOUT 2500)

configure_file(scripts/runtests.in runtests @ONLY)
configure_file(scripts/taxi-env.in taxi-env @ONLY)
