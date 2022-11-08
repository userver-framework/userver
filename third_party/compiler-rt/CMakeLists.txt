project(userver-compiler-rt-parts)

set(SOURCES
  lib/builtins/muloti4.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# workaround for INSTALL dependent libraries without install this static library
set_target_properties (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
