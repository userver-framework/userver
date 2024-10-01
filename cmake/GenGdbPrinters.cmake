find_package(Python3 REQUIRED)

function(gen_gdb_printers TARGET STRUCTURE)
  # Set the path to the Python script
  set (PRINTER_PYTHON_SCRIPT ${USERVER_ROOT_DIR}/scripts/gdb/pretty_printers/${STRUCTURE}.py)
  
  # Set the output header file path in the build directory
  set (OUTPUT_HEADER ${CMAKE_CURRENT_BINARY_DIR}/gdb_helpers/${STRUCTURE}.hpp)

  # Create the output directory if it doesn't exist
  get_filename_component(OUTPUT_HEADER_DIR ${OUTPUT_HEADER} DIRECTORY)
  make_directory(${OUTPUT_HEADER_DIR})

  message(STATUS "Generating ${STRUCTURE} gdb printing header")

  # Generate the header file during configuration
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${USERVER_ROOT_DIR}/scripts/gdb/gen_gdb_printers.py
            ${OUTPUT_HEADER} ${PRINTER_PYTHON_SCRIPT}
    RESULT_VARIABLE GENERATION_RESULT
    OUTPUT_VARIABLE GENERATION_OUTPUT
    ERROR_VARIABLE GENERATION_ERROR
  )

  # Check if the generation was successful
  if(NOT GENERATION_RESULT EQUAL 0)
      message(FATAL_ERROR "Header generation failed: ${GENERATION_ERROR}")
  endif()

  # Add the generated header to the target's sources
  target_sources(${TARGET} PRIVATE ${OUTPUT_HEADER})
  
  # Add the build directory to the target's include directories
  target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

  # Add a custom target to update the .gdbinit file
  set(GDB_INIT_FILE "$ENV{HOME}/.gdbinit")
  set(GDB_SAFE_PATH_RECORD "add-auto-load-safe-path ${CMAKE_BINARY_DIR}")
  set(GDB_INIT_STAMP ${CMAKE_BINARY_DIR}/gdbinit_stamp)

  add_custom_command(
    OUTPUT ${GDB_INIT_STAMP}
    COMMAND grep -cx ${GDB_SAFE_PATH_RECORD} ${GDB_INIT_FILE} > /dev/null || echo ${GDB_SAFE_PATH_RECORD} >> ${GDB_INIT_FILE}
    COMMAND ${CMAKE_COMMAND} -E touch ${GDB_INIT_STAMP}
    COMMENT "Updating ${GDB_INIT_FILE}"
    VERBATIM
  )

  add_custom_target(CONFIGURE_GDBINIT ALL DEPENDS ${GDB_INIT_STAMP})
  add_dependencies(${TARGET} CONFIGURE_GDBINIT)
endfunction()
