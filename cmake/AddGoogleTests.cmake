# Implementation note: public functions here should be usable even without
# a direct include of this script, so the functions should not rely
# on non-cache variables being present.

function(add_google_tests target)
    _userver_setup_environment_validate_impl()
    add_test(
        NAME "${target}"
        COMMAND
        "${target}"
        "--gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${target}.xml"
    )
endfunction()

function(add_google_benchmark_tests target)
    _userver_setup_environment_validate_impl()

    # This is currently a smoke test. That is, we don't try to detect
    # performance regressions, but instead just run 1 loop iteration and check
    # that the executable doesn't crash or hang.
    if (USERVER_CONAN)
      set(BENCHMARK_VERSION ${benchmark_VERSION})
    else()
      set(BENCHMARK_VERSION ${UserverGBench_VERSION})
    endif()
    if(${BENCHMARK_VERSION} VERSION_LESS "1.8.0")
      set(BENCHMARK_MIN_TIME "0")
    else()
      set(BENCHMARK_MIN_TIME "0.0s")
    endif()

    add_test(
        NAME "${target}"
        COMMAND
        "${target}"
        --benchmark_min_time=${BENCHMARK_MIN_TIME}
        --benchmark_color=no
    )
endfunction()
