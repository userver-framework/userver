function(add_google_tests_compile_options target)
    # Google Test uses empty _VA_ARGS_ internally and must be compiled
    # with -Wno-gnu-zero-variadic-macro-arguments
    if (CLANG)
      target_compile_options(${target} PRIVATE
        -Wno-gnu-zero-variadic-macro-arguments
      )
    endif()
endfunction()

function(add_google_tests target)
    add_google_tests_compile_options(${target})
    add_test(NAME ${target} COMMAND ${target}
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${target}.xml
    )
endfunction()

function(add_google_benchmark_tests target)
    # This is currently a smoke test. That is, we don't try to detect
    # performance regressions, but instead just run 1 loop iteration and check
    # that the executable doesn't crash or hang.
    set(BENCHMARK_MIN_TIME 0)

    add_test(NAME ${target} COMMAND ${target}
        --benchmark_min_time=${BENCHMARK_MIN_TIME}
        --benchmark_color=no
    )
endfunction()
