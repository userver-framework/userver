project(yandex-taxi-userver-mongo)

find_package_required_version(libbson-static-1.0
    "libyandex-taxi-mongo-c-driver-dev" 1.13.0)
find_package_required_version(libmongoc-static-1.0
    "libyandex-taxi-mongo-c-driver-dev" 1.13.0)
find_package_required_version(libbsoncxx-static
    "libyandex-taxi-mongo-cxx-driver-dev" 3.3.1)
find_package_required_version(libmongocxx-static
    "libyandex-taxi-mongo-cxx-driver-dev" 3.3.1)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)
list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

file(GLOB_RECURSE MONGO_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_mongotest.cpp
)
list(REMOVE_ITEM SOURCES ${MONGO_TEST_SOURCES})

# file(GLOB_RECURSE BENCH_SOURCES
#   ${CMAKE_CURRENT_SOURCE_DIR}/*_benchmark.cpp
# )
# list (REMOVE_ITEM SOURCES ${BENCH_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    yandex-taxi-userver-core
    ${LIBBSONCXX_STATIC_LIBRARIES}
    ${LIBMONGOCXX_STATIC_LIBRARIES}
  PRIVATE
    ${MONGOC_STATIC_LIBRARIES}
    ${BSON_STATIC_LIBRARIES}
)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
# include them as system to suppress errors
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  ${LIBBSONCXX_STATIC_INCLUDE_DIRS}
  ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}
  ${BSON_STATIC_INCLUDE_DIRS}
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${MONGOC_STATIC_INCLUDE_DIRS}
)
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:yandex-taxi-userver-core,INCLUDE_DIRECTORIES>
)

add_executable(${PROJECT_NAME}_unittest ${UNIT_TEST_SOURCES})
set_target_properties(${PROJECT_NAME}_unittest PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME}_unittest utest ${PROJECT_NAME})
add_google_tests(${PROJECT_NAME}_unittest)

# XXX: testsuite doesn't support parallel builds
if(NOT USERVICES)
  add_executable(${PROJECT_NAME}_mongotest ${MONGO_TEST_SOURCES})
  set_target_properties(${PROJECT_NAME}_mongotest PROPERTIES LINKER_LANGUAGE CXX)
  target_link_libraries(${PROJECT_NAME}_mongotest utest ${PROJECT_NAME})
  add_google_tests_with_mongo(${PROJECT_NAME}_mongotest)
endif(NOT USERVICES)
