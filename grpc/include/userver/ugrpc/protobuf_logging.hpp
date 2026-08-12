#pragma once

/// @file
/// @brief Public API for protobuf logging utilities.

#include <limits>

#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

inline constexpr std::size_t kDefaultLoggingStringLimit = 1024;

/// @brief Convert protobuf message to a limited JSON string for logging.
///
/// The message is serialized to [ProtoJSON](https://protobuf.dev/programming-guides/json/) with the following
/// debugging tweaks:
/// - Fields marked with the `[debug_redact]` option are hidden: their value is replaced with a `[REDACTED]` marker.
/// - Serialization stops early once `limit` bytes have been produced instead of serializing the whole message and
/// only then truncating, which saves CPU on large messages. The already-open JSON containers are closed, so the
/// truncated part before the marker stays a well-formed JSON document.
/// - When truncated, the string ends with a `...(truncated)` marker to indicate that the output was cut off.
///
/// @param message The protobuf message to convert.
/// @param limit Maximum size of the resulting string (excluding the truncation marker).
/// Avoid setting this to very large values as it may cause OOM (Out of Memory) issues.
/// @returns JSON representation of the message, truncated if necessary.
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this (possibly truncated) representation.
/// You CANNOT use it for equality match with reference values in gtest.
///
/// @note This function is `noexcept` and is safe to call from logging code and other no-throw contexts. If @a message
/// cannot be serialized to ProtoJSON, the error is not propagated: the returned string instead has the form
/// `serialization failed: <error description>`.
std::string ToLimitedLoggingString(
    const google::protobuf::Message& message,
    std::size_t limit = kDefaultLoggingStringLimit
) noexcept;

/// @brief Convert protobuf message to an unlimited JSON string for logging.
///
/// Convenience overload equivalent to calling @ref ToLimitedLoggingString with no effective size limit: the whole
/// message is serialized and the result never carries a `...(truncated)` marker. See @ref ToLimitedLoggingString for
/// the exact serialization behavior, the `noexcept` guarantee and the error-handling contract.
///
/// @param message The protobuf message to convert.
/// @returns JSON representation of the whole message.
///
/// @warning Serializes the entire message, so avoid this overload on unbounded input; prefer @ref
/// ToLimitedLoggingString for logging.
inline std::string ToUnlimitedLoggingString(const google::protobuf::Message& message) noexcept {
    return ToLimitedLoggingString(message, /*limit*/ std::numeric_limits<std::size_t>::max());
}

/// @brief Get error details from `grpc::Status` for logging with size limit.
/// @param status The `grpc::Status` to extract details from.
/// @param limit Maximum size of the `"details"` part: forwarded as-is to the nested @ref ToLimitedLoggingString
/// call for the attached `google.rpc.Status`. `code`/`message` are not size-limited.
/// Avoid setting this to very large values as it may cause OOM (Out of Memory) issues.
/// @returns JSON object with a `"code"` key (always present, e.g. `{"code":"OK"}`), plus a `"message"` key when
/// `status.error_message()` is non-empty, plus a `"details"` key when `status` carries a parseable
/// `google.rpc.Status`, holding the JSON produced by
/// @ref ToLimitedLoggingString(const google::protobuf::Message&, std::size_t) for that attached status. For an OK
/// status both `message` and `details` are normally absent, so the result is just `{"code":"OK"}`.
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this representation.
/// You CANNOT use it for equality match with reference values in gtest.
/// @warning If `<details>` ends up truncated (or its own serialization fails), the `...(truncated)` marker (or a
/// `serialization failed: ...` message) is embedded as raw, unquoted text in place of the `"details"` value, which
/// makes the overall result invalid JSON in that case. `code` and `message` are always properly JSON-escaped and
/// cannot break the surrounding JSON.
///
/// @note This function does not itself catch exceptions; its `noexcept` guarantee relies on
/// `formats::json::StringBuilder` and the nested
/// @ref ToLimitedLoggingString(const google::protobuf::Message&, std::size_t) call (used to produce `<details>`)
/// never throwing.
std::string ToLimitedLoggingString(const grpc::Status& status, std::size_t limit = kDefaultLoggingStringLimit) noexcept;

/// @brief Get error details from `grpc::Status` for logging without size limit.
///
/// Convenience overload equivalent to calling @ref ToLimitedLoggingString with no effective size limit: the whole
/// details part is serialized and never carries a `...(truncated)` marker. See @ref ToLimitedLoggingString for the
/// output format, the `noexcept` guarantee and the error-handling contract.
///
/// @param status The `grpc::Status` to extract details from.
/// @returns JSON representation of the status with its full details.
///
/// @warning Serializes the entire details part, so avoid this overload on unbounded input; prefer @ref
/// ToLimitedLoggingString for logging.
inline std::string ToUnlimitedLoggingString(const grpc::Status& status) noexcept {
    return ToLimitedLoggingString(status, /*limit=*/std::numeric_limits<std::size_t>::max());
}

}  // namespace ugrpc

USERVER_NAMESPACE_END

namespace fmt {

/// @brief `fmt::format` support for protobuf messages
template <typename T>
requires std::is_base_of_v<google::protobuf::Message, std::decay_t<T>>
struct formatter<T> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const T& message, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", USERVER_NAMESPACE::ugrpc::ToLimitedLoggingString(message));
    }
};

}  // namespace fmt
