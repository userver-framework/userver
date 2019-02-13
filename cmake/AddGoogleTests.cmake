function(add_google_tests test)
    add_test(${test}
        ${test}
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml
    )
endfunction()

function(add_google_tests_with_mongo test)
    add_test(${test}
        env
            ${CMAKE_BINARY_DIR}/testsuite/taxi-env
            --services=mongo
            run --
            ${CMAKE_CURRENT_BINARY_DIR}/${test}
            --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml
    )
endfunction()

function(add_google_tests_with_postgresql test)
    add_test(${test}
        env
            ${CMAKE_BINARY_DIR}/testsuite/taxi-env
            --services=postgresql
            run --
            ${CMAKE_CURRENT_BINARY_DIR}/${test}
            --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml
    )
endfunction()

function(add_google_tests_with_redis test)
    add_test(${test}
        env
            REDIS_SLEEP_WORKAROUND_SECONDS=12
            ${CMAKE_BINARY_DIR}/testsuite/taxi-env
            --services=redis
            run --
            ${CMAKE_CURRENT_BINARY_DIR}/${test}
            --gtest_output=xml:${CMAKE_BINARY_DIR}/test-results/${test}.xml
    )
endfunction()
