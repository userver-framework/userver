#pragma once

#include <cstddef>

#include <userver/logging/level.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

struct Settings final {
    /// gRPC message body logging level
    logging::Level msg_log_level{logging::Level::kDebug};

    /// Max gRPC message size, the rest will be truncated
    std::size_t max_msg_size{512};

    /// Whether to trim the fields marked as secret from the message
    bool trim_secrets{true};
};

class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(const Settings& settings);

    void OnCallStart(MiddlewareCallContext& context) const override;
    void OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const override;

    void PostRecvMessage(MiddlewareCallContext& context, google::protobuf::Message& request) const override;

    void PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const override;

private:
    Settings settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
