project(userver-easy CXX)

userver_module(
    easy
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES userver::postgresql
    UTEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*_test.cpp"
)

if(USERVER_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()
