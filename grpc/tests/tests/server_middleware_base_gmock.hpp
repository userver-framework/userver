#pragma once

#include <gmock/gmock.h>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests::server {

class ServerMiddlewareBaseMock : public ugrpc::server::MiddlewareBase {
public:
    MOCK_METHOD(void, OnCallStart, (ugrpc::server::MiddlewareCallContext & context), (const, override));

    MOCK_METHOD(
        void,
        PreSendMessage,
        (ugrpc::server::MiddlewareCallContext & context, google::protobuf::Message&),
        (const, override)
    );

    MOCK_METHOD(
        void,
        PostRecvMessage,
        (ugrpc::server::MiddlewareCallContext & context, google::protobuf::Message&),
        (const, override)
    );

    MOCK_METHOD(
        void,
        OnCallFinish,
        (ugrpc::server::MiddlewareCallContext & context, const grpc::Status& status),
        (const, override)
    );
};

}  // namespace tests::server

USERVER_NAMESPACE_END
