option(USERVER_LTO "Use link time optimizations" OFF)

if(NOT USERVER_LTO)
  message(STATUS "LTO: disabled (user request)")
  add_compile_options("-fdata-sections" "-ffunction-sections")

  set(USERVER_IMPL_LTO_FLAGS "-Wl,--gc-sections")
  if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(USERVER_IMPL_LTO_FLAGS "-Wl,-dead_strip -Wl,-dead_strip_dylibs")
  endif()

  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${USERVER_IMPL_LTO_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USERVER_IMPL_LTO_FLAGS}")
else()
  message(STATUS "LTO: on")
  include("${USERVER_CMAKE_DIR}/RequireLTO.cmake")
endif()
