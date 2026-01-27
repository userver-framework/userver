#pragma once

#include <variant>

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
    static MiddlewareHooks StartCallHooks(const google::protobuf::Message* request = nullptr) noexcept;

    static MiddlewareHooks SendMessageHooks(const google::protobuf::Message& send_message) noexcept;

    static MiddlewareHooks RecvMessageHooks(const google::protobuf::Message& recv_message) noexcept;

    static MiddlewareHooks FinishHooks(const grpc::Status& status, const google::protobuf::Message* response = nullptr)
        noexcept;

    void Run(const MiddlewareBase& middleware, MiddlewareCallContext& context) const;

    bool Reverse() const noexcept;

private:
    struct Inbound {
        bool start_call{false};
        const google::protobuf::Message* send_message{};
    };

    struct Outbound {
        const grpc::Status* status{};
        const google::protobuf::Message* recv_message{};
    };

    using Params = std::variant<Inbound, Outbound>;

    explicit MiddlewareHooks(Params&& params) noexcept;

    Params params_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
