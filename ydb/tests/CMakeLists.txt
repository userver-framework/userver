project(userver-ydb-unittest)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_executable(${PROJECT_NAME} ${UNIT_TEST_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
  $<TARGET_PROPERTY:userver-ydb,INCLUDE_DIRECTORIES>
)
target_link_libraries(${PROJECT_NAME} userver-utest userver-ydb)
add_google_tests(${PROJECT_NAME})
