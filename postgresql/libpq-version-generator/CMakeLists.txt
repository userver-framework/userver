project(userver-impl-libpq-version-generator)

add_executable(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE userver-libpq)

option(USERVER_FORCE_PG17_LIGPQ_PATCH OFF)
if (${USERVER_FORCE_PG17_LIGPQ_PATCH})
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/fix_pg17.c")
    target_link_libraries(${PROJECT_NAME} INTERFACE ssl crypto)
endif()

