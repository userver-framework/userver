project(userver-kafka CXX)

if(USERVER_CONAN)
    find_package(RdKafka REQUIRED)
else()
    include(SetupRdKafka)
endif()

userver_module(
    kafka
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    LINK_LIBRARIES_PRIVATE RdKafka::rdkafka
    DBTEST_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    DBTEST_LINK_LIBRARIES userver::kafka-utest
    DEPENDS core
)

target_compile_options(${PROJECT_NAME} PRIVATE "-Wno-ignored-qualifiers")

if(KAFKA_CPM)
    message(STATUS "Including from wrapper ${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include")
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/wrapper/include")
endif()

_userver_directory_install(
    COMPONENT kafka
    FILES "${USERVER_ROOT_DIR}/cmake/SetupRdKafka.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver"
)

_userver_directory_install(
    COMPONENT kafka
    FILES "${USERVER_ROOT_DIR}/cmake/modules/FindRdKafka.cmake" "${USERVER_ROOT_DIR}/cmake/modules/Findlz4.cmake"
          "${USERVER_ROOT_DIR}/cmake/modules/FindSASL2.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/userver/modules"
)

if(USERVER_FEATURE_UTEST)
    add_subdirectory(utest)
endif()

if(USERVER_BUILD_TESTS)
    add_subdirectory(functional_tests)
endif()
