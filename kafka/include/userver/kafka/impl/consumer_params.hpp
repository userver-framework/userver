#pragma once

#include <chrono>

#include <userver/logging/level.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

/// @brief Specifies the logging format for the message key.
enum class MessageKeyLogFormat {
    kPlainText,  ///< Log message key as is
    kHex,        ///< Log message key in hex
};

/// @brief Parameters Consumer uses in runtime.
/// The struct is used only for documentation purposes, Consumer can be
/// created through ConsumerComponent.
struct ConsumerExecutionParams final {
    /// @brief Number of new messages consumer is waiting before calling the
    /// callback.
    /// Maximum number of messages to consume in `poll_timeout` milliseconds.
    std::size_t max_batch_size{1};

    /// @brief Amount of time consumer is waiting for new messages before calling
    /// the callback.
    /// Maximum time to consume `max_batch_size` messages.
    std::chrono::milliseconds poll_timeout{1000};

    /// @brief User callback max duration.
    /// If user callback duration exceeded the `max_callback_duration` consumer is
    /// kicked from its groups and stops working for indefinite time.
    /// @warning On each group membership change, all consumers stop and
    /// start again, so try not to exceed max callback duration.
    std::chrono::milliseconds max_callback_duration{300000};

    /// @brief Time consumer suspends execution after user-callback exception.
    /// @note After consumer restart, all uncommitted messages come again.
    std::chrono::milliseconds restart_after_failure_delay{10000};

    /// @brief Specifies the logging format for the message key.
    /// 'plaintext' - logs the message key as-is, 'hex' - logs in hex.
    MessageKeyLogFormat message_key_log_format{MessageKeyLogFormat::kPlainText};

    /// @brief Log level for everything debug information.
    /// Acceptable values - 'trace', 'debug', 'info', 'warning', 'error', 'critical'
    logging::Level debug_info_log_level{logging::Level::kDebug};

    /// @brief Log level for infos about ordinary actions.
    /// Acceptable values - 'trace', 'debug', 'info', 'warning', 'error', 'critical'
    logging::Level operation_log_level{logging::Level::kInfo};
};

MessageKeyLogFormat Parse(const yaml_config::YamlConfig& config, formats::parse::To<MessageKeyLogFormat>);

}  // namespace kafka::impl

USERVER_NAMESPACE_END
