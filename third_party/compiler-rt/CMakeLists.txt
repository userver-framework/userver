project(userver-compiler-rt-parts)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(SOURCES lib/builtins/muloti4.c)
  add_library(${PROJECT_NAME} STATIC ${SOURCES})

  # workaround for INSTALL dependent libraries without install this static library
  set_target_properties (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

else()
  add_library(${PROJECT_NAME} INTERFACE)
endif()
