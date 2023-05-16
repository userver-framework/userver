#include <ugrpc/server/log_middleware/middleware.hpp>

#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::log_middleware {

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::Handle(MiddlewareCallContext& context) const {
  const auto* request = context.GetInitialRequest();
  if (request) {
    LOG(settings_.log_level)
        << "gRPC message: " << [request, this]() -> std::string {
      const auto& str = request->Utf8DebugString();
      return utils::log::ToLimitedUtf8(str, settings_.max_msg_size);
    }();
  }

  context.Next();
}

}  // namespace ugrpc::server::log_middleware

USERVER_NAMESPACE_END
