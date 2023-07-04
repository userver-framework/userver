project(userver-compiler-rt-parts)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(SOURCES lib/builtins/muloti4.c)
  add_library(${PROJECT_NAME} STATIC ${SOURCES})

  # workaround for INSTALL dependent libraries without install this static library
  if(USERVER_BUILD_SHARED_LIBS)
    set_target_properties (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
  else()
    userver_export(TARGETS userver-compiler-rt-parts)
  endif()
else()
  add_library(${PROJECT_NAME} INTERFACE)
  if(NOT USERVER_BUILD_SHARED_LIBS)
    userver_export(TARGETS userver-compiler-rt-parts)
  endif()
endif()
