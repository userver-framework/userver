project (userver-samples-json2yaml)

file (GLOB_RECURSE SOURCES *.cpp)

add_executable (${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME} userver-universal)

# /// [add_test]
add_test (NAME ${PROJECT_NAME}-pytest
  COMMAND ${USERVER_PYTHON} -m pytest --test-binary=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
# /// [add_test]
