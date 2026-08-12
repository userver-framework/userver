PROGRAM(userver-functional-test-service)

ALLOCATOR(J)

SUBSCRIBER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/chaotic-openapi
    taxi/uservices/userver/samples/chaotic_openapi_service/src
)

SRCS(
    main.cpp
)

STYLE_YAML()

END()

RECURSE_FOR_TESTS(
    testsuite
)
