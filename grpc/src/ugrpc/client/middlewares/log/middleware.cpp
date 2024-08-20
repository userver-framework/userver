#include "middleware.hpp"

#include <userver/logging/log.hpp>
#include <userver/utils/log.hpp>

#include <ugrpc/impl/protobuf_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

namespace {

std::string ToLogString(const google::protobuf::Message& message,
                        std::size_t max_size) {
  std::unique_ptr<google::protobuf::Message> trimmed{message.New()};
  trimmed->CopyFrom(message);
  ugrpc::impl::TrimSecrets(*trimmed);
  return utils::log::ToLimitedUtf8(trimmed->Utf8DebugString(), max_size);
}

}  // namespace

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

void Middleware::PreSendMessage(
    MiddlewareCallContext& /*context*/,
    const google::protobuf::Message& message) const {
  LOG(settings_.log_level) << "gRPC send message: "
                           << ToLogString(message, settings_.max_msg_size);
}

void Middleware::PostRecvMessage(
    MiddlewareCallContext& /*context*/,
    const google::protobuf::Message& message) const {
  LOG(settings_.log_level) << "gRPC recv message: "
                           << ToLogString(message, settings_.max_msg_size);
}

MiddlewareFactory::MiddlewareFactory(const Settings& settings)
    : settings_(settings) {}

std::shared_ptr<const MiddlewareBase> MiddlewareFactory::GetMiddleware(
    std::string_view /*client_name*/) const {
  return std::make_shared<Middleware>(settings_);
}

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
