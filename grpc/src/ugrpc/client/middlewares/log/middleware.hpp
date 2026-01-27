#pragma once

#include <cstddef>

#include <userver/logging/level.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

struct Settings final {
    /// Set `LOG` level threshold.
    /// Only `LOG`s with level HIGHER OR EQUAL TO the threshold will be evaluated.
    ///
    /// For example, if the log_level is @ref logging::Level::kInfo, then:
    /// @code
    ///   LOG_INFO() << "This message will be logged.";
    ///   LOG_DEBUG() << "This message will NOT be logged.";
    /// @endcode
    ///
    /// e.g. Request and Response messages are logged with `msg_log_level` level.
    /// It can be disabled with `log_level` set higher than `msg_log_level`.
    /// Whole rpc status is logged with level `INFO`, and `WARNING` in case of rpc fails.
    logging::Level log_level{logging::Level::kDebug};

    /// Logging level for Request and Response messages
    logging::Level msg_log_level{logging::Level::kDebug};

    /// Max message size to log, the rest will be truncated
    std::size_t max_msg_size{512};
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
