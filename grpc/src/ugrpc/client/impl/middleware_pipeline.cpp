#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

void MiddlewarePipeline::PreStartCall(CallState& state) {
    MiddlewareCallContext context{state};
    for (const auto& mw : state.GetMiddlewares()) {
        mw->PreStartCall(context);
    }
}

void MiddlewarePipeline::PreSendMessage(CallState& state, const google::protobuf::Message& message) {
    MiddlewareCallContext context{state};
    for (const auto& mw : state.GetMiddlewares()) {
        mw->PreSendMessage(context, message);
    }
}

void MiddlewarePipeline::PostRecvMessage(CallState& state, const google::protobuf::Message& message) {
    MiddlewareCallContext context{state};
    for (const auto& mw : state.GetMiddlewares()) {
        mw->PostRecvMessage(context, message);
    }
}

void MiddlewarePipeline::PostFinish(CallState& state) {
    const auto& status = state.GetStatus();
    MiddlewareCallContext context{state};
    for (const auto& mw : state.GetMiddlewares()) {
        mw->PostFinish(context, status);
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
