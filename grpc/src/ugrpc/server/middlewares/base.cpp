#include <userver/ugrpc/server/middlewares/base.hpp>

#include <userver/components/component.hpp>

#include <ugrpc/impl/internal_tag.hpp>
#include <userver/ugrpc/server/impl/exceptions.hpp>
#include <userver/utils/impl/internal_tag.hpp>

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

/////////////////////////////////////////////////////////////////////////////////////

MiddlewareCallContext::MiddlewareCallContext(
    utils::impl::InternalTag,
    impl::CallAnyBase& call,
    dynamic_config::Snapshot&& config
)
    : CallContextBase(utils::impl::InternalTag{}, call), config_(std::move(config)) {}

void MiddlewareCallContext::SetError(grpc::Status&& status) noexcept {
    UASSERT(!status.ok());
    if (!status.ok()) {
        *status_ = std::move(status);
    }
}

bool MiddlewareCallContext::IsClientStreaming() const noexcept {
    return impl::IsClientStreaming(GetCall(utils::impl::InternalTag{}).GetCallKind());
}

bool MiddlewareCallContext::IsServerStreaming() const noexcept {
    return impl::IsServerStreaming(GetCall(utils::impl::InternalTag{}).GetCallKind());
}

const dynamic_config::Snapshot& MiddlewareCallContext::GetInitialDynamicConfig() const {
    UASSERT(config_.has_value());
    return config_.value();
}

ugrpc::impl::RpcStatisticsScope& MiddlewareCallContext::GetStatistics(ugrpc::impl::InternalTag tag) {
    return GetCall(utils::impl::InternalTag{}).GetStatistics(tag);
}

void MiddlewareCallContext::SetStatusPtr(grpc::Status* status) {
    UASSERT(status);
    status_ = status;
}

grpc::Status& MiddlewareCallContext::GetStatus() {
    UASSERT(status_);
    return *status_;
}

/////////////////////////////////////////////////////////////////////////////////////

MiddlewarePipelineComponent::MiddlewarePipelineComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : USERVER_NAMESPACE::middlewares::impl::AnyMiddlewarePipelineComponent(
          config,
          context,
          {/*middlewares=*/{
              {"grpc-server-logging", {}},
              {"grpc-server-baggage", {}},
              {"grpc-server-congestion-control", {}},
              {"grpc-server-deadline-propagation", {}},
              {"grpc-server-headers-propagator", {}},
          }}
      ) {}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
