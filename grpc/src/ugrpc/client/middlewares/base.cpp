#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

void MiddlewareCallContext::Next() {
  UASSERT_MSG(user_call_, "MiddlewareCallContext must be used only once");

  if (middleware_ == middleware_end_) {
    user_call_();
    user_call_ = {};
  } else {
    // NOLINTNEXTLINE(readability-qualified-auto)
    const auto middleware = middleware_++;
    UASSERT(*middleware);
    (*middleware)->Handle(*this);

    UINVARIANT(!user_call_, "Middleware forgot to call context.Next()?");
  }
}

CallAnyBase& MiddlewareCallContext::GetCall() { return call_; }

const ::google::protobuf::Message* MiddlewareCallContext::GetInitialRequest() {
  return request_;
}

MiddlewareFactoryBase::~MiddlewareFactoryBase() = default;

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
