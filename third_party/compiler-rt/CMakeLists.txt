cmake_minimum_required(VERSION 3.8)

project(userver-compiler-rt-parts)

set(SOURCES
  lib/builtins/muloti4.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
