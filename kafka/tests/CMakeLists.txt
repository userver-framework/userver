file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})
target_link_libraries(${PROJECT_NAME}-unittest PRIVATE userver::kafka userver::utest)
target_include_directories(${PROJECT_NAME}-unittest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
add_google_tests(${PROJECT_NAME}-unittest)
