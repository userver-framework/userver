#include <userver/ugrpc/server/middlewares/base.hpp>

#include <userver/components/component.hpp>

#include <userver/ugrpc/server/impl/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

MiddlewareCallContext::MiddlewareCallContext(
    const Middlewares& middlewares,
    CallAnyBase& call,
    utils::function_ref<void()> user_call,
    const dynamic_config::Snapshot& config,
    ::google::protobuf::Message* request
)
    : middleware_(middlewares.begin()),
      middleware_end_(middlewares.end()),
      user_call_(std::move(user_call)),
      call_(call),
      config_(config),
      request_(request) {}

void MiddlewareCallContext::Next() {
    if (is_called_from_handle_) {
        // It is important for non-stream calls
        if (request_) {
            (*middleware_)->CallRequestHook(*this, *request_);
            if (call_.IsFinished()) throw impl::MiddlewareRpcInterruptionError();
        }
        ++middleware_;
    }
    if (middleware_ == middleware_end_) {
        ClearMiddlewaresResources();
        user_call_();
    } else {
        is_called_from_handle_ = true;
        (*middleware_)->Handle(*this);
    }
}

bool MiddlewareCallContext::IsClientStreaming() const noexcept { return impl::IsClientStreaming(call_.GetCallKind()); }

bool MiddlewareCallContext::IsServerStreaming() const noexcept { return impl::IsServerStreaming(call_.GetCallKind()); }

CallAnyBase& MiddlewareCallContext::GetCall() const { return call_; }

void MiddlewareCallContext::ClearMiddlewaresResources() {
    UASSERT(config_);
    config_.reset();
}

const dynamic_config::Snapshot& MiddlewareCallContext::GetInitialDynamicConfig() const {
    UASSERT(config_);
    return config_.value();
}

void MiddlewareBase::CallRequestHook(const MiddlewareCallContext&, google::protobuf::Message&) {}

void MiddlewareBase::CallResponseHook(const MiddlewareCallContext&, google::protobuf::Message&) {}

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
