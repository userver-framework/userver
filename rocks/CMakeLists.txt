project(userver-rocks CXX)

file(GLOB_RECURSE SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

find_library(ROCKSDB_LIBRARY rocksdb)

if(ROCKSDB_LIBRARY)
    target_link_libraries(${PROJECT_NAME} ${ROCKSDB_LIBRARY})
else()
    message(FATAL_ERROR "RocksDB library not found")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
