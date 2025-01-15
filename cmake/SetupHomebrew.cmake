option(USERVER_USE_BREW_LIBRARIES "(MacOS) Find package preferes libraries installed with homebrew and make linker search libraries in brew folders" ON)

if(NOT DEFINED $CACHE{USERVER_BREW_PREFIX})
    find_program(BREW_BIN brew)
    if(BREW_BIN)
      execute_process(
        COMMAND ${BREW_BIN} --prefix
        OUTPUT_VARIABLE brew_prefix
        RESULT_VARIABLE brew_prefix_result
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      if(brew_prefix_result EQUAL 0)
        set(USERVER_BREW_PREFIX "${brew_prefix}" CACHE INTERNAL "Brew prefix")
        message(STATUS "brew prefix is: ${USERVER_BREW_PREFIX}")
      endif()
    endif()
endif()

if (USERVER_BREW_PREFIX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${USERVER_BREW_PREFIX}/lib")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L${USERVER_BREW_PREFIX}/lib")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Foundation")

    if(NOT USERVER_BREW_PREFIX IN_LIST CMAKE_PREFIX_PATH AND NOT USERVER_CONAN)
      set(CMAKE_PREFIX_PATH "${USERVER_BREW_PREFIX}" ${CMAKE_PREFIX_PATH} PARENT_SCOPE)
    endif()
endif()
