PROGRAM(userver-functional-test-service)

ALLOCATOR(J)

SUBSCRIBER(g:taxi-common)

SRCDIR(taxi/uservices/userver/samples/grpc_middleware_service/src)

ADDINCL(
    taxi/uservices/userver/samples/grpc_middleware_service/src
)

PEERDIR(
    contrib/libs/grpc
    contrib/libs/grpc/grpcpp_channelz
    contrib/libs/protobuf
    taxi/uservices/userver/samples/grpc_middleware_service/proto
    taxi/uservices/userver/core
    taxi/uservices/userver/grpc
)

SRCS(
    main.cpp
    client/view.cpp
    service/view.cpp
    http_handlers/say-hello/view.cpp
    middlewares/client/component.cpp
    middlewares/client/middleware.cpp
    middlewares/server/component.cpp
    middlewares/server/middleware.cpp
    middlewares/auth.cpp
)

END()

RECURSE_FOR_TESTS(tests)
