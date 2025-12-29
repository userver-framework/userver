#pragma once

/// @file
/// @brief Public API for protobuf logging utilities.

#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

inline constexpr std::size_t kDefaultDebugStringLimit = 1024;

/// @brief Convert protobuf message to limited debug string for logging.
///
/// Differences from built-in `DebugString`:
/// - Fields marked with `[debug_redact]` option are hidden (`DebugString` only does so since Protobuf v30).
/// - When the character limit is reached, serialization stops immediately (`DebugString` still wastes CPU on
/// serializing the entire message regardless).
/// - When truncated, the string ends with `...(truncated)` marker to indicate that the output was cut off.
///
/// @param message The protobuf message to convert.
/// @param limit Maximum size of the resulting string.
/// Avoid setting this to very large values as it may cause OOM (Out of Memory) issues.
/// @returns String representation of the message, truncated if necessary.
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this debug text representation.
/// You CANNOT use it for equality match with reference values in gtest.
std::string ToLimitedDebugString(const google::protobuf::Message& message, std::size_t limit);

/// @brief Convert protobuf message to unlimited debug string for logging.
///
/// Differences from built-in `DebugString`:
/// - Fields marked with `[debug_redact]` option are hidden (`DebugString` only does so since Protobuf v30).
///
/// @param message The protobuf message to convert.
/// @returns String representation of the message.
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this debug text representation.
/// You CANNOT use it for equality match with reference values in gtest.
std::string ToUnlimitedDebugString(const google::protobuf::Message& message);

/// @brief Get error details from `grpc::Status` for logging with size limit.
/// @param status The `grpc::Status` to extract details from.
/// @param max_size Maximum size of the error details part.
/// Avoid setting this to very large values as it may cause OOM (Out of Memory) issues.
/// @returns String representation of error details, formatted as "code: {code}, error message: {message}\nerror
/// details:\n{details}". The error details part may be truncated if it exceeds max_size, in which case it ends with
/// `...(truncated)` marker.
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this debug text representation.
/// You CANNOT use it for equality match with reference values in gtest.
std::string ToLimitedDebugString(const grpc::Status& status, std::size_t max_size = kDefaultDebugStringLimit);

/// @brief Get error details from `grpc::Status` for logging without size limit.
/// @param status The `grpc::Status` to extract details from.
/// @returns String representation of error details.
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this debug text representation.
/// You CANNOT use it for equality match with reference values in gtest.
std::string ToUnlimitedDebugString(const grpc::Status& status);

}  // namespace ugrpc

USERVER_NAMESPACE_END

namespace fmt {

/// @brief `fmt::format` support for protobuf messages
template <typename T>
struct formatter<T, typename std::enable_if_t<std::is_base_of_v<google::protobuf::Message, std::decay_t<T>>, char>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const T& message, FormatContext& ctx) const {
        return fmt::format_to(
            ctx.out(),
            "{}",
            USERVER_NAMESPACE::ugrpc::ToLimitedDebugString(message, USERVER_NAMESPACE::ugrpc::kDefaultDebugStringLimit)
        );
    }
};

}  // namespace fmt
