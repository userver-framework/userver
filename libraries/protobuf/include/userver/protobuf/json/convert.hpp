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
/// @throws PrintError if conversion has failed
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If protobuf enum value has multiple aliases (`allow_alias` enum option is on) then the first alias in the
///       definition order is outputted.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
[[nodiscard]] formats::json::ValueBuilder MessageToJsonBuilder(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
);

/// @brief Converts protobuf @a message to JSON `Value`.
/// @throws PrintError if conversion has failed
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If protobuf enum value has multiple aliases (`allow_alias` enum option is on) then the first alias in the
///       definition order is outputted.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
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
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @note If conversion fails, @a message is left in a valid but unspecified state.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
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
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
template <
    typename T,
    typename = std::enable_if_t<
        std::is_base_of_v<::google::protobuf::Message, T> || !std::is_same_v<::google::protobuf::Message, T>>>
[[nodiscard]] T JsonToMessage(const formats::json::Value& json, const ParseOptions& options = {}) {
    T message;
    protobuf::json::JsonToMessage(json, message, options);
    return message;
}

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
