project(userver-samples-mongo-support CXX)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} userver::core userver::mongo)

# /// [testsuite - cmake]
userver_testsuite_add_simple(
    PYTHON_BINARY "${TESTSUITE_PYTHON_BINARY}"
)
# /// [testsuite - cmake]
