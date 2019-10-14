include(FindPython)

message(STATUS "Run userver codegen ...")
execute_process(
    COMMAND 
        ${PYTHON} ${userver_SOURCE_DIR}/generator.py 
        --build-dir ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE RESULT
    WORKING_DIRECTORY ${userver_SOURCE_DIR})

if (RESULT)
    message(FATAL_ERROR 
        "Run userver codegen failed with exit code: ${RESULT}"
    )
endif(RESULT)
