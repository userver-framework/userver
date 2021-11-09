function(add_google_tests_compile_options target)
    # Google Test uses empty _VA_ARGS_ internally and must be compiled
    # with -Wno-gnu-zero-variadic-macro-arguments
    if (CLANG)
      target_compile_options(${target} PRIVATE
        -Wno-gnu-zero-variadic-macro-arguments
      )
    endif()
endfunction()

function(add_google_tests test)
    add_google_tests_compile_options(${test})
    add_test(${test}
        ${test}
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml
    )
endfunction()
