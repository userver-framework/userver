#include <userver/ugrpc/server/middlewares/base.hpp>

#include <userver/components/component.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/server/impl/call_kind.hpp>
#include <userver/ugrpc/server/impl/call_state.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

void MiddlewareBase::OnCallStart(MiddlewareCallContext&) const {}

void MiddlewareBase::OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const {
    if (!status.ok()) {
        return context.SetError(grpc::Status{status});
    }
}

void MiddlewareBase::PostRecvMessage(MiddlewareCallContext&, google::protobuf::Message&) const {}

void MiddlewareBase::PreSendMessage(MiddlewareCallContext&, google::protobuf::Message&) const {}

MiddlewareCallContext::MiddlewareCallContext(utils::impl::InternalTag, impl::CallState& state)
    : CallContextBase(utils::impl::InternalTag{}, state) {}

void MiddlewareCallContext::SetError(grpc::Status&& status) noexcept {
    UASSERT(!status.ok());
    if (!status.ok()) {
        status_ = std::move(status);
    }
}

bool MiddlewareCallContext::IsClientStreaming() const noexcept {
    return impl::IsClientStreaming(GetCallState(utils::impl::InternalTag{}).call_kind);
}

bool MiddlewareCallContext::IsServerStreaming() const noexcept {
    return impl::IsServerStreaming(GetCallState(utils::impl::InternalTag{}).call_kind);
}

const dynamic_config::Snapshot& MiddlewareCallContext::GetInitialDynamicConfig() const {
    const auto& config_snapshot = GetCallState(utils::impl::InternalTag{}).config_snapshot;
    UINVARIANT(config_snapshot.has_value(), "GetInitialDynamicConfig can only be called in OnCallStart");
    return *config_snapshot;
}

ugrpc::impl::RpcStatisticsScope& MiddlewareCallContext::GetStatistics(utils::impl::InternalTag) {
    return GetCallState(utils::impl::InternalTag{}).statistics_scope;
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
