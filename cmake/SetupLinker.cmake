set(USERVER_USE_LD "" CACHE STRING "Linker to use e.g. gold, lld")

if (USERVER_USE_LD)
  execute_process(COMMAND "${CMAKE_C_COMPILER}" "-fuse-ld=${USERVER_USE_LD}" -Wl,--version
      ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)

  if ((USERVER_USE_LD MATCHES "gold") AND (LD_VERSION MATCHES "GNU gold"))
    set(CUSTOM_LD_OK ON CACHE INTERNAL CUSTOM_LD_OK)
    message(STATUS "Using GNU gold linker")
  elseif((USERVER_USE_LD MATCHES "lld") AND (LD_VERSION MATCHES "LLD"))
    set(CUSTOM_LD_OK ON CACHE INTERNAL CUSTOM_LD_OK)
    message(STATUS "Using LLVM lld linker")
  endif()

  if (CUSTOM_LD_OK)
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-fuse-ld=${USERVER_USE_LD}")
    list(APPEND CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=${USERVER_USE_LD}")
  else()
    message(STATUS "Custom linker isn't available, using the default system linker.")
  endif()
endif()
