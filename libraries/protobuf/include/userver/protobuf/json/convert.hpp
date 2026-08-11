#pragma once

/// @file userver/protobuf/json/convert.hpp
/// @brief Utilities for converting protobuf messages to/from JSON `ValueBuilder`/`Value` according to
///        [ProtoJSON](https://protobuf.dev/programming-guides/json) format.

#include <cstddef>
#include <string>
#include <type_traits>

#include <google/protobuf/message.h>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/protobuf/json/convert_options.hpp>
#include <userver/protobuf/json/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json {

/// @brief Converts protobuf @a message to JSON `ValueBuilder`.
/// @throws PrintError if conversion has failed
/// The conversion is performed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If protobuf enum value has multiple aliases (`allow_alias` enum option is on) then the first alias in the
///       definition order is outputted.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatibility with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
[[nodiscard]] formats::json::ValueBuilder MessageToJsonBuilder(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
);

/// @brief Converts protobuf @a message to JSON `Value`.
/// @throws PrintError if conversion has failed
/// The conversion is performed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If protobuf enum value has multiple aliases (`allow_alias` enum option is on) then the first alias in the
///       definition order is outputted.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatibility with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
[[nodiscard]] inline formats::json::Value MessageToJson(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
) {
    return protobuf::json::MessageToJsonBuilder(message, options).ExtractValue();
}

/// @brief Converts @a json to protobuf @a message .
/// @throws ParseError if conversion has failed
/// @throws MemberMissingException is @a json holds nothing
/// The conversion is performed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If conversion fails, @a message is left in a valid but unspecified state.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatibility with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
void JsonToMessage(
    const formats::json::Value& json,
    ::google::protobuf::Message& message,
    const ParseOptions& options = {}
);

/// @brief Converts @a json to protobuf message of type `T`.
/// @tparam T protobuf message type
/// @throws ParseError if conversion has failed
/// @throws MemberMissingException is @a json holds nothing
/// The conversion is performed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatibility with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
template <typename T>
requires(std::is_base_of_v<::google::protobuf::Message, T> || !std::is_same_v<::google::protobuf::Message, T>)
[[nodiscard]] T JsonToMessage(const formats::json::Value& json, const ParseOptions& options = {}) {
    T message;
    protobuf::json::JsonToMessage(json, message, options);
    return message;
}

/// @brief Serializes protobuf @a message to a JSON string.
///
/// Honors @ref PrintOptions in exactly the same way as @ref MessageToJson / @ref MessageToJsonBuilder do (including
/// `nonportable_raw_any`, which controls whether `google.protobuf.Any` is expanded or emitted raw). In particular,
/// `[debug_redact = true]` fields are **not** redacted here either, same as in @ref MessageToJson /
/// @ref MessageToJsonBuilder.
///
/// @param message The protobuf message to convert.
/// @param options Same conversion options as for @ref MessageToJson / @ref MessageToJsonBuilder.
/// @returns ProtoJSON representation of @a message.
/// @throws PrintError if conversion has failed
std::string MessageToJsonString(const ::google::protobuf::Message& message, const PrintOptions& options);

/// @brief Serializes protobuf @a message to a JSON string for debugging/logging, stopping early once @a limit bytes
/// have been produced.
///
/// Unlike @ref MessageToJson / @ref MessageToJsonBuilder / @ref MessageToJsonString, this is a **debug** serializer:
/// - Field names are always the proto field names (`preserve_proto_field_names`), not `json_name`, to match the
///   `.proto` definition.
/// - Fields marked with the `[debug_redact = true]` option are hidden: their value is replaced with a `"[REDACTED]"`
///   marker.
/// - `google.protobuf.Any` is expanded (like @ref MessageToJsonString with default options), but falls back to the
///   raw representation instead of failing when the payload type can't be resolved in the descriptor pool or parsed.
/// - Serialization stops early once `limit` bytes have been produced instead of serializing the whole message and
///   only then truncating, which saves CPU on large messages. The already-open JSON containers are closed, so the
///   truncated part before the marker stays a well-formed JSON document.
///
/// @param message The protobuf message to convert.
/// @param limit Maximum size of the resulting string. The output may exceed it by at most one scalar value, since
/// the traversal is interrupted at container-element granularity. This function does not append any truncation
/// marker itself; callers that need one (e.g. for logging) should detect truncation by checking whether the result
/// size exceeds `limit`.
/// @returns JSON representation of @a message, truncated to (approximately) @a limit bytes if necessary.
/// @throws PrintError if conversion has failed
///
/// @warning This is a debug representation of protobuf that is unstable and should only be used for diagnostics.
/// The order of keys in maps is unstable; the format itself can change even within a single run.
/// You CANNOT parse back from this (possibly truncated) representation.
/// You CANNOT use it for equality match with reference values in gtest.
std::string MessageToDebugString(const ::google::protobuf::Message& message, std::size_t limit);

}  // namespace protobuf::json

/* NOTE !
   Currently this breaks linkage because similar functions are defined in the
   userver/grpc/include/userver/ugrpc/proto_json.hpp . When those functions are
   removed as legacy, uncomment this code (and do not forget to uncomment tests
   in the userver/libraries/protobuf/tests/json/complex_from_json_test.cpp and
   taxi/uservices/userver/libraries/protobuf/tests/json/complex_to_json_test.cpp).

namespace formats::serialize {

/// @brief Conversion from any `google::protobuf::Message` to @ref formats::json::Value.
/// Uses the ProtoJSON format in the same way as @ref protobuf::MessageToJson called with a default options.
///
/// Use as:
/// @code{.cpp}
/// auto json = formats::json::ValueBuilder{message}.ExtractValue();
/// @endcode
json::Value Serialize(const google::protobuf::Message& message, To<json::Value>);

}  // namespace formats::serialize

namespace formats::parse {

/// @brief Conversion from @ref formats::json::Value to `google::protobuf::Message`.
/// Uses the ProtoJSON format in the same way as @ref protobuf::JsonToMessage called with a default options.
///
/// Use as:
/// @code{.cpp}
/// auto value = json.As<google::protobuf::Value>();
/// @endcode
template <
    typename TMessage,
    typename = std::enable_if_t<
        std::is_base_of_v<::google::protobuf::Message, TMessage> &&
        !std::is_same_v<::google::protobuf::Message, TMessage>>>
TMessage Parse(const json::Value& value, To<TMessage>) {
    return protobuf::json::JsonToMessage<TMessage>(value);
}

}  // namespace formats::parse

*/

USERVER_NAMESPACE_END
