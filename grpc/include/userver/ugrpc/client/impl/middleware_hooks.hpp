#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {
class MiddlewareBase;
class MiddlewareCallContext;
}  // namespace ugrpc::client

namespace ugrpc::client::impl {

class MiddlewareHooks {
public:
    void SetStartCall() noexcept;
    void SetSendMessage(const google::protobuf::Message& send_message) noexcept;
    void SetRecvMessage(const google::protobuf::Message& recv_message) noexcept;
    void SetStatus(const grpc::Status& status) noexcept;

    void Run(const MiddlewareBase& middleware, MiddlewareCallContext& context) const;

private:
    bool start_call_{false};
    const google::protobuf::Message* send_message_{};
    const google::protobuf::Message* recv_message_{};
    const grpc::Status* status_{};
};

MiddlewareHooks StartCallHooks(const google::protobuf::Message* request = nullptr) noexcept;
MiddlewareHooks SendMessageHooks(const google::protobuf::Message& send_message) noexcept;
MiddlewareHooks RecvMessageHooks(const google::protobuf::Message& recv_message) noexcept;
MiddlewareHooks FinishHooks(const grpc::Status& status, const google::protobuf::Message* response = nullptr) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
