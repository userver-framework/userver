project(userver-kafka-utest)

file(GLOB_RECURSE KAFKA_LIB_UTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

add_library(${PROJECT_NAME} STATIC ${KAFKA_LIB_UTEST_SOURCES})
target_link_libraries(${PROJECT_NAME}
    PUBLIC userver::kafka userver::utest
)
target_include_directories(${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(KAFKA_CPM)
    target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../wrapper/include
    )
endif()

_userver_directory_install(COMPONENT kafka
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/.."
)
_userver_install_targets(COMPONENT kafka TARGETS ${PROJECT_NAME})
