include_guard(GLOBAL)

file(WRITE ${CMAKE_BINARY_DIR}/empty.hpp
    "// File to suppress empty sources error in CMake"
)

function(add_empty_object_target target_name)
    add_library(${target_name}
        OBJECT
        ${CMAKE_BINARY_DIR}/empty.hpp # Suppress empty sources error
    )
endfunction()
