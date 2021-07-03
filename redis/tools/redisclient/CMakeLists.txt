cmake_minimum_required (VERSION 3.8)

project (redisclient_perf)

file (GLOB_RECURSE SOURCES *.cpp)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable (${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME}
    Boost::program_options
    yandex-taxi-userver+standalone_engine
    userver-redis
)

# Include directories marked SYSTEM so that includes from external projects
# do not generate warnings treated as errors
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:userver-redis,INCLUDE_DIRECTORIES>)
