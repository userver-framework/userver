#include <userver/ugrpc/server/impl/rpc.hpp>

#include <boost/range/adaptor/reversed.hpp>

#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

std::unique_lock<engine::SingleWaitingTaskMutex> ResponderBase::TakeMutexIfBidirectional() {
    if (state_.call_kind == impl::CallKind::kBidirectionalStream) {
        // In stream -> stream RPCs, Recv and Send hooks will naturally run in parallel.
        // However, this can cause data races when:
        // * accessing StorageContext;
        // * accessing Span (AddTag);
        // * accessing ServerContext (e.g. setting metadata);
        //
        // For now, this mutex lock mitigates most of these issues.
        // There is still some data race potential remaining:
        // * if a PostRecvMessage hook writes to StorageContext and user Send task reads from the same key in parallel,
        //   or vice versa, or same with PreSendMessage hook;
        // * if user code sets metadata in ServerContext in parallel with a PostRecvMessage or PreSendMessage middleware
        //   hook.
        return std::unique_lock(mutex_);
    }
    return {};
}

void ResponderBase::ApplyRequestHook(google::protobuf::Message& request) {
    auto lock = TakeMutexIfBidirectional();
    MiddlewareCallContext middleware_call_context{utils::impl::InternalTag{}, state_};

    for (const auto& middleware : state_.middlewares) {
        middleware->PostRecvMessage(middleware_call_context, request);
        auto& status = middleware_call_context.GetStatus(utils::impl::InternalTag{});
        if (!status.ok()) {
            throw impl::MiddlewareRpcInterruptionError(std::move(status));
        }
    }
}

void ResponderBase::ApplyResponseHook(google::protobuf::Message& response) {
    auto lock = TakeMutexIfBidirectional();
    MiddlewareCallContext middleware_call_context{utils::impl::InternalTag{}, state_};

    for (const auto& middleware : boost::adaptors::reverse(state_.middlewares)) {
        middleware->PreSendMessage(middleware_call_context, response);
        auto& status = middleware_call_context.GetStatus(utils::impl::InternalTag{});
        if (!status.ok()) {
            throw impl::MiddlewareRpcInterruptionError(std::move(status));
        }
    }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
