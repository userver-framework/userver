cmake_minimum_required (VERSION 3.5)

set (PROJECT httpclient_perf)
project (${PROJECT} VERSION ${PROJECT_VERSION_STRING})

file (GLOB_RECURSE SOURCES *.cpp)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable (${PROJECT} ${SOURCES})
target_link_libraries (${PROJECT} 
    yandex-taxi-userver
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${JEMALLOC_LIBRARIES})
