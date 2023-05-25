#include <userver/ugrpc/client/log_middleware/middleware.hpp>

#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::log_middleware {

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

MiddlewareFactory::MiddlewareFactory(const Middleware::Settings& settings)
    : settings_(settings) {}

std::shared_ptr<const MiddlewareBase> MiddlewareFactory::GetMiddleware(
    std::string_view /*client_name*/) const {
  return std::make_shared<Middleware>(settings_);
}

}  // namespace ugrpc::client::log_middleware

USERVER_NAMESPACE_END
