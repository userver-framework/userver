#pragma once

#include <gmock/gmock.h>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests::server {

class ServerMiddlewareBaseMock : public ugrpc::server::MiddlewareBase {
public:
    MOCK_METHOD(void, OnCallStart, (ugrpc::server::MiddlewareCallContext&), (const, override));

    MOCK_METHOD(
        void,
        PostRecvMessage,
        (ugrpc::server::MiddlewareCallContext&, google::protobuf::Message&),
        (const, override)
    );

    MOCK_METHOD(
        void,
        PreSendMessage,
        (ugrpc::server::MiddlewareCallContext&, google::protobuf::Message&),
        (const, override)
    );

    MOCK_METHOD(void, PreSendStatus, (ugrpc::server::MiddlewareCallContext&, grpc::Status&), (const, override));

    MOCK_METHOD(
        void,
        OnCallFinish,
        (ugrpc::server::MiddlewareCallContext&, const std::optional<grpc::Status>&),
        (const, override)
    );
};

}  // namespace tests::server

USERVER_NAMESPACE_END
