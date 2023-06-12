#include <userver/ugrpc/server/middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

void MiddlewareCallContext::Next() {
  if (middleware_ == middleware_end_) {
    user_call_();
  } else {
    // NOLINTNEXTLINE(readability-qualified-auto)
    const auto middleware = middleware_++;
    (*middleware)->Handle(*this);
  }
}

CallAnyBase& MiddlewareCallContext::GetCall() { return call_; }

const ::google::protobuf::Message* MiddlewareCallContext::GetInitialRequest() {
  return request_;
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
