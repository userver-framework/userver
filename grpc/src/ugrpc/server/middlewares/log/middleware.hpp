#pragma once

#include <cstddef>

#include <boost/container/flat_map.hpp>

#include <userver/logging/level.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

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
    /// Whole rpc status is logged with level `INFO`, and `WARNING` or `ERROR` in case of rpc fails.
    logging::Level log_level{logging::Level::kDebug};

    /// Logging level for Request and Response messages
    logging::Level msg_log_level{logging::Level::kDebug};

    /// Max message size to log, the rest will be truncated
    std::size_t max_msg_size{512};

    /// Local log level of the server span
    /// It applies to logs in user-provided handler
    /// @ref tracing::Span::SetLocalLogLevel
    logging::Level local_log_level{logging::Level::kDebug};

    /// map of "status_code": log_level items to override span log level for specific status codes
    /// see @ref ugrpc::kStatusCodesMap for available statuses
    boost::container::flat_map<grpc::StatusCode, logging::Level> status_codes_log_level;
};

class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(const Settings& settings);

    void OnCallStart(MiddlewareCallContext& context) const override;

    void PostRecvMessage(MiddlewareCallContext& context, google::protobuf::Message& request) const override;

    void PreSendMessage(MiddlewareCallContext& context, google::protobuf::Message& response) const override;

    void OnCallFinish(MiddlewareCallContext& context, const std::optional<grpc::Status>& status) const override;

private:
    Settings settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
