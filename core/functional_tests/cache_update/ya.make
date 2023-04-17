PROGRAM(userver-functional-test-service)

ALLOCATOR(J)

OWNER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/core
)

SRCS(
    service.cpp
)

END()

RECURSE_FOR_TESTS(tests)
