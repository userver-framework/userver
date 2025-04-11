#pragma once

#include <cstddef>

#include <userver/logging/level.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

struct Settings {
    /// gRPC message body logging level
    logging::Level msg_log_level{logging::Level::kDebug};

    /// Max gRPC message size, the rest will be truncated
    std::size_t max_msg_size{512};

    /// Whether to trim the fields marked as secret from the message
    bool trim_secrets{true};
};

/// [MiddlewareBase example declaration]
/// @brief middleware for RPC handler logging settings
class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(const Settings& settings);

    void PreStartCall(MiddlewareCallContext& context) const override;

    void PreSendMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const override;

    void PostRecvMessage(MiddlewareCallContext& context, const google::protobuf::Message& message) const override;

    void PostFinish(MiddlewareCallContext& context, const grpc::Status& status) const override;

private:
    Settings settings_;
};
/// [MiddlewareBase example declaration]

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
