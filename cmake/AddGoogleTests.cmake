function(add_google_tests test)
    # Google Test uses empty _VA_ARGS_ internally and must be compiled
    # with -Wno-gnu-zero-variadic-macro-arguments
    target_compile_options(${test} PRIVATE
      -Wno-gnu-zero-variadic-macro-arguments)
    add_test(${test}
        ${test}
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml
    )
endfunction()
