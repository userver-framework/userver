PROGRAM(userver-functional-test-service)

ALLOCATOR(J)

OWNER(g:taxi-common)

PEERDIR(
    taxi/uservices/userver/core
    taxi/uservices/userver/redis
)

SRCS(
    redis_service.cpp
)

END()

RECURSE_FOR_TESTS(tests)
