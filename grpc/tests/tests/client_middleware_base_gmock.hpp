#pragma once

#include <gmock/gmock.h>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests::client {

class ClientMiddlewareBaseMock : public ugrpc::client::MiddlewareBase {
public:
    MOCK_METHOD(void, PreStartCall, (ugrpc::client::MiddlewareCallContext&), (const, override));

    MOCK_METHOD(
        void,
        PreSendMessage,
        (ugrpc::client::MiddlewareCallContext&, const google::protobuf::Message&),
        (const, override)
    );

    MOCK_METHOD(
        void,
        PostRecvMessage,
        (ugrpc::client::MiddlewareCallContext&, const google::protobuf::Message&),
        (const, override)
    );

    MOCK_METHOD(void, PostFinish, (ugrpc::client::MiddlewareCallContext&, const grpc::Status&), (const, override));
};

}  // namespace tests::client

USERVER_NAMESPACE_END
