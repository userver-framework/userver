project(userver-librseq)

SET(SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/rseq.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)

add_library(${PROJECT_NAME} OBJECT ${SOURCES})

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
_userver_install_targets(COMPONENT core TARGETS ${PROJECT_NAME})
