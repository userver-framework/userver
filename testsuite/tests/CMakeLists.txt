project(userver-testsuite-tests)

add_subdirectory(chaos)
add_subdirectory(config)
add_subdirectory(metrics)
add_subdirectory(sql_coverage_empty)
add_subdirectory(tskv)
# add_subdirectory(sql_coverage_full) TODO: TAXICOMMON-10381
