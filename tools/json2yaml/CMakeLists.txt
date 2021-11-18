project (json2yaml)

file (GLOB_RECURSE SOURCES *.cpp)

add_executable (${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME}
    userver-universal
)
