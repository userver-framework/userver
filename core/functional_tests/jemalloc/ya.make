PROGRAM(userver-functional-test-service)

ALLOCATOR(J)

SUBSCRIBER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/core
)

SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(tests)
