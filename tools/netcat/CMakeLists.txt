project(userver-tool-netcat CXX)

file(GLOB_RECURSE SOURCES *.cpp)

find_package(Boost REQUIRED CONFIG COMPONENTS program_options)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    userver-core
    Boost::program_options
)
