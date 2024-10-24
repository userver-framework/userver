find_package(Python3 REQUIRED)

function(gen_gdb_printers TARGET STRUCTURE)
  # Set the path to the Python script
  set (PRINTER_PYTHON_SCRIPT "${USERVER_ROOT_DIR}/scripts/gdb/pretty_printers/${STRUCTURE}/printers.py")
  set (GDB_AUTOGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gdb_autogen")

  # Set the output header file path in the build directory
  set (OUTPUT_HEADER "${GDB_AUTOGEN_DIR}/gdb_autogen/${STRUCTURE}/printers.hpp")

  # Create the output directory if it doesn't exist
  get_filename_component(OUTPUT_HEADER_DIR "${OUTPUT_HEADER}" DIRECTORY)
  make_directory("${OUTPUT_HEADER_DIR}")

  message(STATUS "Generating ${STRUCTURE} gdb printing header")

  # Generate the header file during configuration
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${USERVER_ROOT_DIR}/scripts/gdb/gen_gdb_printers.py"
            "${OUTPUT_HEADER}" "${PRINTER_PYTHON_SCRIPT}"
    RESULT_VARIABLE GENERATION_RESULT
    OUTPUT_VARIABLE GENERATION_OUTPUT
    ERROR_VARIABLE GENERATION_ERROR
  )

  # Check if the generation was successful
  if(NOT GENERATION_RESULT EQUAL 0)
      message(FATAL_ERROR "Header generation failed: ${GENERATION_ERROR}")
  endif()

  # Add the generated header to the target's sources
  target_sources(${TARGET} PRIVATE "${OUTPUT_HEADER}")

  target_include_directories(${TARGET} PRIVATE "${GDB_AUTOGEN_DIR}")
endfunction()
