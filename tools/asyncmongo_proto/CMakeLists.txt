cmake_minimum_required (VERSION 3.8)

project (asyncmongo_proto)

file (GLOB_RECURSE SOURCES *.cpp)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package_required_version(libmongoc-static-1.0
    "libyandex-taxi-mongo-c-driver-dev" 1.12.0)

add_executable (${PROJECT_NAME} ${SOURCES})
target_link_libraries (${PROJECT_NAME}
    yandex-taxi-userver+standalone_engine
    Boost::program_options
    ${MONGOC_STATIC_LIBRARIES}
)

target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
  ${MONGOC_STATIC_INCLUDE_DIRS}
)
