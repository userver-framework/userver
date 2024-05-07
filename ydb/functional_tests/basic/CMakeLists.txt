project(userver-ydb-tests-basic CXX)

add_executable(${PROJECT_NAME} 
  "ydb_service.cpp"
  "views/describe-table/post/view.cpp"
  "views/select-list/post/view.cpp"
  "views/select-rows/post/view.cpp"
  "views/upsert-row/post/view.cpp"
)
target_link_libraries(${PROJECT_NAME} userver-ydb)
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
userver_chaos_testsuite_add(TESTS_DIRECTORY tests-metrics)
