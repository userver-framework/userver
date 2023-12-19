#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

MiddlewareCallContext::MiddlewareCallContext(
    const Middlewares& middlewares, CallAnyBase& call,
    utils::function_ref<void()> user_call,
    const ::google::protobuf::Message* request)
    : middleware_(middlewares.begin()),
      middleware_end_(middlewares.end()),
      user_call_(user_call),
      call_(call),
      request_(request) {}

void MiddlewareCallContext::Next() {
  if (middleware_ == middleware_end_) {
    (*user_call_)();
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
