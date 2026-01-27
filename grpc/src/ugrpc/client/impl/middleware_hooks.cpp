#include <userver/ugrpc/client/impl/middleware_hooks.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

MiddlewareHooks MiddlewareHooks::StartCallHooks(const google::protobuf::Message* request) noexcept {
    return MiddlewareHooks{Inbound{true, request}};
}

MiddlewareHooks MiddlewareHooks::SendMessageHooks(const google::protobuf::Message& send_message) noexcept {
    return MiddlewareHooks{Inbound{false, &send_message}};
}

MiddlewareHooks MiddlewareHooks::FinishHooks(const grpc::Status& status, const google::protobuf::Message* response)
    noexcept {
    UASSERT(status.ok() || nullptr == response);
    return MiddlewareHooks{Outbound{&status, response}};
}

MiddlewareHooks MiddlewareHooks::RecvMessageHooks(const google::protobuf::Message& recv_message) noexcept {
    return MiddlewareHooks{Outbound{nullptr, &recv_message}};
}

void MiddlewareHooks::Run(const MiddlewareBase& middleware, MiddlewareCallContext& context) const {
    std::visit(
        utils::Overloaded{
            [&middleware, &context](Inbound params) {
                if (params.start_call) {
                    middleware.PreStartCall(context);
                }
                if (params.send_message) {
                    middleware.PreSendMessage(context, *params.send_message);
                }
            },
            [&middleware, &context](Outbound params) {
                if (params.recv_message) {
                    middleware.PostRecvMessage(context, *params.recv_message);
                }
                if (params.status) {
                    middleware.PostFinish(context, *params.status);
                }
            },
        },
        params_
    );
}

bool MiddlewareHooks::Reverse() const noexcept { return std::holds_alternative<Outbound>(params_); }

MiddlewareHooks::MiddlewareHooks(Params&& params) noexcept
        : params_{std::move(params)}
{}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
