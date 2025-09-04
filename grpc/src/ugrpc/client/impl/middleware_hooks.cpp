#include <userver/ugrpc/client/impl/middleware_hooks.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void MiddlewareHooks::SetStartCall() noexcept { start_call_ = true; }

void MiddlewareHooks::SetSendMessage(const google::protobuf::Message& send_message) noexcept {
    send_message_ = &send_message;
}

void MiddlewareHooks::SetRecvMessage(const google::protobuf::Message& recv_message) noexcept {
    recv_message_ = &recv_message;
}

void MiddlewareHooks::SetStatus(const grpc::Status& status) noexcept { status_ = &status; }

void MiddlewareHooks::Run(const MiddlewareBase& middleware, MiddlewareCallContext& context) const {
    if (start_call_) {
        middleware.PreStartCall(context);
    }

    if (send_message_) {
        middleware.PreSendMessage(context, *send_message_);
    }

    if (recv_message_) {
        middleware.PostRecvMessage(context, *recv_message_);
    }

    if (status_) {
        middleware.PostFinish(context, *status_);
    }
}

MiddlewareHooks StartCallHooks(const google::protobuf::Message* request) noexcept {
    MiddlewareHooks hooks;
    hooks.SetStartCall();
    if (request) {
        hooks.SetSendMessage(*request);
    }
    return hooks;
}

MiddlewareHooks SendMessageHooks(const google::protobuf::Message& send_message) noexcept {
    MiddlewareHooks hooks;
    hooks.SetSendMessage(send_message);
    return hooks;
}

MiddlewareHooks RecvMessageHooks(const google::protobuf::Message& recv_message) noexcept {
    MiddlewareHooks hooks;
    hooks.SetRecvMessage(recv_message);
    return hooks;
}

MiddlewareHooks FinishHooks(const grpc::Status& status, const google::protobuf::Message* response) noexcept {
    MiddlewareHooks hooks;
    if (status.ok() && response) {
        hooks.SetRecvMessage(*response);
    }
    hooks.SetStatus(status);
    return hooks;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
