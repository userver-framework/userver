function(add_google_tests PROJECT)
    add_test(${PROJECT} ${PROJECT} --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${PROJECT}.xml)
endfunction()
