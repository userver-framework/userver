project(userver-librseq C)

SET(SOURCES
    src/rseq.c
    src/rseq-mempool.c
    src/smp.c
)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-implicit-function-declaration)
target_compile_definitions(${PROJECT_NAME} PRIVATE _GNU_SOURCE)
set_target_properties(${PROJECT_NAME} PROPERTIES C_EXTENSIONS ON)
_userver_install_targets(COMPONENT core TARGETS ${PROJECT_NAME})
