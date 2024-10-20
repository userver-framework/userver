#pragma once

/// @file userver/server/middlewares/builtin.hpp
/// @brief Names of userver's built-in HTTP server middlewares

#include <string_view>

USERVER_NAMESPACE_BEGIN

/// Names of userver's built-in HTTP server middlewares
namespace server::middlewares::builtin {

inline constexpr std::string_view kHandlerMetrics = "userver-handler-metrics-middleware";
inline constexpr std::string_view kTracing = "userver-tracing-middleware";
inline constexpr std::string_view kSetAcceptEncoding = "userver-set-accept-encoding-middleware";
inline constexpr std::string_view kUnknownExceptionsHandling = "userver-unknown-exceptions-handling-middleware";
inline constexpr std::string_view kRateLimit = "userver-rate-limit-middleware";
inline constexpr std::string_view kDeadlinePropagation = "userver-deadline-propagation-middleware";
inline constexpr std::string_view kBaggage = "userver-baggage-middleware";
inline constexpr std::string_view kAuth = "userver-auth-middleware";
inline constexpr std::string_view kDecompression = "userver-decompression-middleware";
inline constexpr std::string_view kExceptionsHandling = "userver-exceptions-handling-middleware";

}  // namespace server::middlewares::builtin

USERVER_NAMESPACE_END
