#include "middleware.hpp"

#include <userver/tracing/span.hpp>
#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::Handle(MiddlewareCallContext& context) const {
  const auto* request = context.GetInitialRequest();
  if (request) {
    LOG(settings_.msg_log_level)
        << "gRPC message: " << [request, this]() -> std::string {
      const auto& str = request->Utf8DebugString();
      return utils::log::ToLimitedUtf8(str, settings_.max_msg_size);
    }();
  }

  if (settings_.local_log_level) {
    auto& span = tracing::Span::CurrentSpan();
    span.SetLocalLogLevel(*settings_.local_log_level);
  }

  context.Next();
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
