#pragma once

/// @file userver/server/handlers/json_body_logging.hpp
/// @brief JSON body logging helpers for generated HTTP handlers.

#include <optional>
#include <string>
#include <string_view>

#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

struct HandlerConfig;

/// @brief Options for selecting JSON body logging settings.
struct JsonBodyLoggingOptions final {
    /// Optional handler lookup key override for @ref USERVER_HTTP_SERVER_LOGS.
    /// When unset, uses the low-cardinality path from @ref server::handlers::HandlerConfig.
    std::optional<std::string_view> handler_name_override;
};

/// @brief Apply the request JSON masking policy and limit to a logging hook's output.
///
/// Masking rules are configured in @ref USERVER_HTTP_SERVER_LOGS.
/// Configured bodies are processed as JSON. Existing hooks must run first.
/// Returns masked JSON or a UTF-8-safe prefix with `...(truncated, total N bytes)`.
/// The prefix and suffix together fit the limit; N is the input byte size after custom hooks.
/// If the suffix itself does not fit, returns only its leading bytes.
/// With no policy, returns the body unchanged.
/// Invalid JSON produces an error marker.
/// Final UTF-8 limiting belongs to @ref utils::log::ToLimitedUtf8.
/// @ref server::handlers::JsonBodyLoggingOptions::handler_name_override may select settings by a custom key.
/// Slash-prefixed keys are validated as static handler paths; other custom keys are not validated.
/// The override limit is applied while masking;
/// final limiting of unmasked bodies still uses the configured handler path.
std::string MaskJsonRequestBodyForLogging(
    const std::string& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config,
    JsonBodyLoggingOptions options = {}
);

/// @overload
/// Reuses the input string when no masking policy is configured.
std::string MaskJsonRequestBodyForLogging(
    std::string&& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config,
    JsonBodyLoggingOptions options = {}
);

/// @brief Apply the response JSON masking policy and limit.
///
/// Explicit policies apply to typed and non-schema response bodies alike.
/// @copydetails server::handlers::MaskJsonRequestBodyForLogging
std::string MaskJsonResponseBodyForLogging(
    const std::string& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config,
    JsonBodyLoggingOptions options = {}
);

/// @overload
/// Reuses the input string when no masking policy is configured.
std::string MaskJsonResponseBodyForLogging(
    std::string&& body,
    const dynamic_config::Snapshot& snapshot,
    const HandlerConfig& config,
    JsonBodyLoggingOptions options = {}
);

}  // namespace server::handlers

USERVER_NAMESPACE_END
