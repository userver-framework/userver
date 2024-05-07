project(userver-samples-ydb_service CXX)

add_executable(${PROJECT_NAME}
  "ydb_service.cpp"
  "views/bson-reading/post/view.cpp"
  "views/bson-upserting/post/view.cpp"
  "views/select-rows/post/view.cpp"
  "views/upsert-rows/post/view.cpp"
  "views/upsert-2rows/post/view.cpp"
  "views/upsert-row/post/view.cpp"
  "components/topic_reader.cpp"
)
target_link_libraries(${PROJECT_NAME} userver::mongo userver::ydb)
target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

userver_sample_testsuite_add()
