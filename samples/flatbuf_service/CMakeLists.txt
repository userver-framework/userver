project(userver-samples-flatbuf_service CXX)

set(FLATC_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/flatbuffer_schema.fbs.h)
add_custom_command(
  COMMENT "Generating FlatBuffer header flatbuffer_schema.fbs.h"
  COMMAND flatc
  ARGS
      --cpp --gen-object-api --filename-suffix '.fbs'
      -o ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffer_schema.fbs
  OUTPUT ${FLATC_OUTPUT}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/flatbuffer_schema.fbs
)

add_executable(${PROJECT_NAME} flatbuf_service.cpp ${FLATC_OUTPUT})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} userver-core)

userver_sample_testsuite_add()
