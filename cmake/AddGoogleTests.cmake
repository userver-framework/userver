function(add_google_tests test)
    add_test(${test} ${test} --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml)
endfunction()
