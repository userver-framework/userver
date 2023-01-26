project (dns-resolver)

file (GLOB_RECURSE SOURCES *.cpp)

find_package(Boost REQUIRED COMPONENTS program_options)

add_executable (${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME}
    userver-core
    Boost::program_options
)

# Include directories marked SYSTEM so that includes from external projects
# do not generate warnings treated as errors
target_include_directories (${PROJECT_NAME} SYSTEM PRIVATE
    $<TARGET_PROPERTY:userver-core,INCLUDE_DIRECTORIES>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE SPDLOG_FMT_EXTERNAL=1)
