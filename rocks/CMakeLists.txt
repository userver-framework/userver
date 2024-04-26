project(userver-rocks CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
)

file(GLOB_RECURSE UNIT_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp
)

list(REMOVE_ITEM SOURCES ${UNIT_TEST_SOURCES})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)


include(SetupRocksDeps)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    userver-core
    RocksDB::rocksdb
)

if (USERVER_IS_THE_ROOT_PROJECT)
    add_executable(${PROJECT_NAME}-unittest ${UNIT_TEST_SOURCES})

    target_include_directories(${PROJECT_NAME}-unittest PRIVATE
        $<TARGET_PROPERTY:${PROJECT_NAME},INCLUDE_DIRECTORIES>
    )

    target_link_libraries(${PROJECT_NAME}-unittest PRIVATE
      userver-core
      userver-utest
      userver-rocks
    )

    add_google_tests(${PROJECT_NAME}-unittest)
endif()
