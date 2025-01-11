set(USERVER_USE_LD_DEFAULT "")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  string(REPLACE "." ";" COMPILER_VERSION_LIST "${CMAKE_CXX_COMPILER_VERSION}")
  list(GET COMPILER_VERSION_LIST 0 COMPILER_VERSION_MAJOR)

  if (NOT DEFINED $CACHE{USERVER_IMPL_HAS_LLD})
    execute_process(
        COMMAND sh "-c" "command -v lld-${COMPILER_VERSION_MAJOR}"
        OUTPUT_VARIABLE LLD_PATH
    )
    if (LLD_PATH)
      set(LLD_FOUND ON)
    else()
      set(LLD_FOUND OFF)
    endif()
    set(USERVER_IMPL_HAS_LLD "${LLD_FOUND}" CACHE INTERNAL "Whether the system has lld linker")
  endif()

  if (USERVER_IMPL_HAS_LLD)
    set(USERVER_USE_LD_DEFAULT "lld-${COMPILER_VERSION_MAJOR}")
  endif()
endif()

set(USERVER_USE_LD "${USERVER_USE_LD_DEFAULT}" CACHE STRING "Linker to use e.g. gold, lld")

if (USERVER_USE_LD)
  execute_process(COMMAND "${CMAKE_C_COMPILER}" "-fuse-ld=${USERVER_USE_LD}" -Wl,--version
      ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)

  if ((USERVER_USE_LD MATCHES "gold") AND (LD_VERSION MATCHES "GNU gold"))
    set(CUSTOM_LD_OK ON CACHE INTERNAL CUSTOM_LD_OK)
    message(STATUS "Using GNU gold linker")
  elseif((USERVER_USE_LD MATCHES "lld") AND (LD_VERSION MATCHES "LLD"))
    set(CUSTOM_LD_OK ON CACHE INTERNAL CUSTOM_LD_OK)
    message(STATUS "Using LLVM lld linker: ${USERVER_USE_LD}")
  endif()

  if (CUSTOM_LD_OK)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=${USERVER_USE_LD}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=${USERVER_USE_LD}")
  else()
    message(STATUS "Custom linker isn't available, using the default system linker.")
  endif()
else()
  message(STATUS "Using the default system linker, probably GNU ld")
endif()
