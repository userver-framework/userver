project(userver-compiler-rt-parts)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(SOURCES lib/builtins/muloti4.c)
  add_library(${PROJECT_NAME} STATIC ${SOURCES})
else()
  add_library(${PROJECT_NAME} INTERFACE)
endif()
