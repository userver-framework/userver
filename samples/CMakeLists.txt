cmake_minimum_required(VERSION 3.12)
project(userver-samples)

file(GLOB_RECURSE SAMPLE_APPLICATIONS "*.cpp")
foreach(SAMPLE ${SAMPLE_APPLICATIONS})
  get_filename_component(SAMPLE_BASENAME ${SAMPLE} NAME_WE)
  set(SAMPLE_NAME ${PROJECT_NAME}-${SAMPLE_BASENAME})

  add_executable(${SAMPLE_NAME} ${SAMPLE})
  set_target_properties(${SAMPLE_NAME} PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(${SAMPLE_NAME} yandex-taxi-userver-core)
endforeach()

add_test(
  NAME ${PROJECT_NAME}-test
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/run_samples.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
