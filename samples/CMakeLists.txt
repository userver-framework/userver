project(userver-samples CXX)

add_custom_target(${PROJECT_NAME})

file(GLOB_RECURSE SAMPLE_APPLICATIONS "*.cpp")
foreach(SAMPLE ${SAMPLE_APPLICATIONS})
  get_filename_component(SAMPLE_BASENAME ${SAMPLE} NAME_WE)
  set(SAMPLE_NAME ${PROJECT_NAME}-${SAMPLE_BASENAME})

  add_executable(${SAMPLE_NAME} ${SAMPLE})
  target_link_libraries(${SAMPLE_NAME} userver-core)

  add_dependencies(${PROJECT_NAME} ${SAMPLE_NAME})
endforeach()

target_link_libraries(userver-samples-postgres_service userver-postgresql)

add_test(
  NAME ${PROJECT_NAME}-test
  COMMAND ${CMAKE_BINARY_DIR}/testsuite/taxi-env
      --services=postgresql
      run -- python3 ${CMAKE_CURRENT_SOURCE_DIR}/run_samples.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(${PROJECT_NAME}-test PROPERTIES TIMEOUT 30)
