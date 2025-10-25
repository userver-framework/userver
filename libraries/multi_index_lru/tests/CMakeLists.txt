project(multi-index-lru-tests)

find_package(Boost REQUIRED)
find_package(benchmark REQUIRED)

file(GLOB_RECURSE MULTIINDEX_LRU_LIB_TESTS_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_library(${PROJECT_NAME} STATIC ${MULTIINDEX_LRU_LIB_TESTS_SOURCES})
target_link_libraries(${PROJECT_NAME} 
    PUBLIC 
        userver::multi-index-lru 
        benchmark::benchmark 
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
add_executable(multi-index-lru-test-runner 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp  
)

target_link_libraries(multi-index-lru-test-runner 
    PRIVATE
        ${PROJECT_NAME}
)

_userver_directory_install(
    COMPONENT multi-index-lru
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

_userver_install_targets(COMPONENT multi-index-lru TARGETS ${PROJECT_NAME})