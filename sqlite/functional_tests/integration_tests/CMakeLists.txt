project(userver-sqlite-tests-basic CXX)

add_library(
    ${PROJECT_NAME}_objs OBJECT src/handlers/key_value.hpp src/handlers/key_value.cpp src/handlers/batch.hpp
                                src/handlers/batch.cpp src/db/sql.hpp
)

target_include_directories(${PROJECT_NAME}_objs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME}_objs userver::sqlite)

add_executable(${PROJECT_NAME} "service.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

userver_chaos_testsuite_add(TESTS_DIRECTORY tests)
