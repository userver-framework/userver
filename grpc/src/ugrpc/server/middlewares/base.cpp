#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

void MiddlewareCallContext::Next() {
  if (middleware_ == middleware_end_) {
    ClearMiddlewaresResources();
    user_call_();
  } else {
    // NOLINTNEXTLINE(readability-qualified-auto)
    const auto middleware = middleware_++;
    (*middleware)->Handle(*this);
  }
}

void MiddlewareCallContext::ClearMiddlewaresResources() {
  UASSERT(config_);
  config_.reset();
}

CallAnyBase& MiddlewareCallContext::GetCall() { return call_; }

std::string_view MiddlewareCallContext::GetServiceName() const {
  return service_name_;
}

std::string_view MiddlewareCallContext::GetMethodName() const {
  return method_name_;
}

const dynamic_config::Snapshot& MiddlewareCallContext::GetInitialDynamicConfig()
    const {
  UASSERT(config_);
  return config_.value();
}

const ::google::protobuf::Message* MiddlewareCallContext::GetInitialRequest() {
  return request_;
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
