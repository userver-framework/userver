include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/UserverVenv.cmake")

function(gen_gdb_printers TARGET STRUCTURE)
  # Set the path to the Python script
  set(PRINTER_PYTHON_SCRIPT "${USERVER_ROOT_DIR}/scripts/gdb/pretty_printers/${STRUCTURE}/printers.py")
  set(GDB_AUTOGEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gdb_autogen")

  # Set the output header file path in the build directory
  set(OUTPUT_HEADER "${GDB_AUTOGEN_DIR}/gdb_autogen/${STRUCTURE}/printers.hpp")

  # Create the output directory if it doesn't exist
  get_filename_component(OUTPUT_HEADER_DIR "${OUTPUT_HEADER}" DIRECTORY)
  file(MAKE_DIRECTORY "${OUTPUT_HEADER_DIR}")

  message(STATUS "Generating ${STRUCTURE} gdb printing header")

  # Generate the header file during build
  _userver_initialize_codegen_flag()
  add_custom_command(
      OUTPUT "${OUTPUT_HEADER}"
      COMMAND "${USERVER_PYTHON_PATH}" "${USERVER_ROOT_DIR}/scripts/gdb/gen_gdb_printers.py"
              "${OUTPUT_HEADER}" "${PRINTER_PYTHON_SCRIPT}"
      DEPENDS "${USERVER_ROOT_DIR}/scripts/gdb/gen_gdb_printers.py" "${PRINTER_PYTHON_SCRIPT}"
      COMMENT "Generating pretty printers at ${OUTPUT_HEADER}"
      ${CODEGEN}
  )

  # Add the generated header to the target's sources
  target_sources(${TARGET} PRIVATE "${OUTPUT_HEADER}")

  target_include_directories(${TARGET} PRIVATE "${GDB_AUTOGEN_DIR}")
endfunction()
