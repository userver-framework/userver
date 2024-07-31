#include "middleware.hpp"

#include <userver/utils/log.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::Handle(MiddlewareCallContext& context) const {
  const auto* request = context.GetInitialRequest();
  if (request) {
    std::unique_ptr<google::protobuf::Message> trimmed{request->New()};
    trimmed->CopyFrom(*request);
    ugrpc::impl::TrimSecrets(*trimmed);
    LOG(settings_.log_level)
        << "gRPC message: "
        << utils::log::ToLimitedUtf8(trimmed->Utf8DebugString(),
                                     settings_.max_msg_size);
  }
  context.Next();
}

MiddlewareFactory::MiddlewareFactory(const Settings& settings)
    : settings_(settings) {}

std::shared_ptr<const MiddlewareBase> MiddlewareFactory::GetMiddleware(
    std::string_view /*client_name*/) const {
  return std::make_shared<Middleware>(settings_);
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
