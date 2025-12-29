#pragma once

/// @file userver/protobuf/json/convert.hpp
/// @brief Utilities for converting protobuf messages to/from JSON `ValueBuilder`/`Value` according to
///        [ProtoJSON](https://protobuf.dev/programming-guides/json) format.

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

/// @brief Converts protobuf @a message to JSON `ValueBuider`.
/// @throws WriteError if writing protobuf message to JSON has failed
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If protobuf enum value has multiple aliases (`allow_alias` enum option is on) then the first alias in the
///       definition order is outputted.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested. At least extension fields and `required` label
///          are not supported, but also some other `proto2`-specific parts of the syntax may not be correctly handled.
[[nodiscard]] formats::json::ValueBuilder MessageToJsonBuilder(
    const ::google::protobuf::Message& message,
    const WriteOptions& options = {}
);

/// @brief Converts protobuf @a message to JSON `Value`.
/// @throws WriteError if writing protobuf message to JSON has failed
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If protobuf enum value has multiple aliases (`allow_alias` enum option is on) then the first alias in the
///       definition order is outputted.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested. At least extension fields and `required` label
///          are not supported, but also some other `proto2`-specific parts of the syntax may not be correctly handled.
[[nodiscard]] inline formats::json::Value MessageToJson(
    const ::google::protobuf::Message& message,
    const WriteOptions& options = {}
) {
    return protobuf::json::MessageToJsonBuilder(message, options).ExtractValue();
}

/// @brief Converts @a json to protobuf @a message .
/// @throws ReadError if reading protobuf message from JSON has failed
/// @throws MemberMissingException is @a json holds nothing
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If conversion fails, @a message is left in a valid but unspecified state.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested. At least extension fields and `required` label
///          are not supported, but also some other `proto2`-specific parts of the syntax may not be correctly handled.
void JsonToMessage(const formats::json::Value& json, const ReadOptions& options, ::google::protobuf::Message& message);

/// @brief Converts @a json to protobuf message of type `T`.
/// @tparam T protobuf message type
/// @throws ReadError if reading protobuf message from JSON has failed
/// @throws MemberMissingException is @a json holds nothing
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested. At least extension fields and `required` label
///          are not supported, but also some other `proto2`-specific parts of the syntax may not be correctly handled.
template <
    typename T,
    typename = std::enable_if_t<
        std::is_base_of_v<::google::protobuf::Message, T> || !std::is_same_v<::google::protobuf::Message, T>>>
[[nodiscard]] T JsonToMessage(const formats::json::Value& json, const ReadOptions& options = {}) {
    T message;
    protobuf::json::JsonToMessage(json, options, message);
    return message;
}

}  // namespace protobuf::json

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

USERVER_NAMESPACE_END
