cmake_minimum_required (VERSION 3.5)

set (PROJECT redisclient_perf)
project (${PROJECT} VERSION ${PROJECT_VERSION_STRING})

file (GLOB_RECURSE SOURCES *.cpp)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable (${PROJECT} ${SOURCES})
target_link_libraries (${PROJECT}
    -Wl,--start-group
    yandex-taxi-userver
    yandex-taxi-backend-common-cpp
    -Wl,--end-group
    Boost::program_options
    )

# FIXME: delete after spdlog uses target
target_include_directories(${PROJECT} SYSTEM PRIVATE
  ${SPDLOG_INCLUDE_DIRS}
)
