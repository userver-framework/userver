cmake_minimum_required(VERSION 3.12...3.31)
project(service_template CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(
    userver
    COMPONENTS core #
               grpc # grpc template current
               mongo # mongo template current
               postgresql # postgresql template current
    QUIET
)
if(NOT userver_FOUND)
    # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
    download_userver(TRY_DIR third_party/userver)
endif()

userver_setup_environment()

# Common sources
include_directories(src)

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/greeting.cpp #
    src/hello.cpp #
    src/hello_postgres.cpp # postgresql template current
    src/hello_mongo.cpp # mongo template current
    src/hello_grpc.cpp # grpc template current
)
target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core #
           userver::grpc # grpc template current
           userver::mongo # mongo template current
           userver::postgresql # postgresql template current
)

# grpc template on Create a proto library with userver extensions
userver_add_grpc_library(${PROJECT_NAME}_proto PROTOS handlers/hello.proto)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC ${PROJECT_NAME}_proto)
# grpc template off

# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# Unit Tests
add_executable(${PROJECT_NAME}_unittest src/greeting_test.cpp)
target_link_libraries(${PROJECT_NAME}_unittest PRIVATE ${PROJECT_NAME}_objs userver::utest)
add_google_tests(${PROJECT_NAME}_unittest)

# Benchmarks
add_executable(${PROJECT_NAME}_benchmark src/greeting_benchmark.cpp)
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE ${PROJECT_NAME}_objs userver::ubench)
add_google_benchmark_tests(${PROJECT_NAME}_benchmark)

# Functional testing
userver_testsuite_add_simple()

# Install
include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
    message(STATUS "Set install prefix: $ENV{PREFIX}")
    file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
    set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(
    TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)
install(
    FILES ${CONFIGS_FILES}
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)
