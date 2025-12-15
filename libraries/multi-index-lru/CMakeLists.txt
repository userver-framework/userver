project(userver-multi-index-lru CXX)

find_package(Boost REQUIRED)

userver_module(
    multi-index-lru
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
    UBENCH_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_benchmark.cpp"
    DEPENDS core
)

target_include_directories(
    userver-multi-index-lru
    PUBLIC ${Boost_INCLUDE_DIRS}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests/"
)
