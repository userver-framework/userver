project(userver-samples CXX)

add_custom_target(${PROJECT_NAME})

file(GLOB SAMPLE_APPLICATIONS "*")
foreach(SAMPLE ${SAMPLE_APPLICATIONS})
  if (EXISTS "${SAMPLE}/CMakeLists.txt")
    get_filename_component(SAMPLE_BASENAME ${SAMPLE} NAME)
    set(SAMPLE_NAME ${PROJECT_NAME}-${SAMPLE_BASENAME})

    add_subdirectory(${SAMPLE_BASENAME})
    add_dependencies(${PROJECT_NAME} ${SAMPLE_NAME})
  endif()
endforeach()

add_test(
  NAME ${PROJECT_NAME}-test
  COMMAND ${CMAKE_BINARY_DIR}/testsuite/taxi-env
      --services postgresql redis mongo --log-level debug
      run -- python3 ${CMAKE_CURRENT_SOURCE_DIR}/run_samples.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
set_tests_properties(${PROJECT_NAME}-test PROPERTIES TIMEOUT 30)
