#pragma once

/// @file userver/ugrpc/proto_json.hpp
/// @brief Utilities for conversion Protobuf -> Json
/// @ingroup userver_formats_serialize userver_formats_parse

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace impl {

extern const google::protobuf::util::JsonPrintOptions kDefaultJsonPrintOptions;
extern const google::protobuf::util::JsonParseOptions kDefaultJsonParseOptions;

void FromJsonStringImpl(
    std::string_view json_string,
    google::protobuf::Message& output,
    const google::protobuf::util::JsonParseOptions& options
);

}  // namespace impl

/// @brief Returns formats::json::Value representation of protobuf message
/// @throws formats::json::Exception
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::MessageToJson .
formats::json::Value MessageToJson(const google::protobuf::Message& message);

/// @brief Returns formats::json::Value representation of protobuf message
/// @throws formats::json::Exception
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::MessageToJson .
formats::json::Value MessageToJson(
    const google::protobuf::Message& message,
    const google::protobuf::util::JsonPrintOptions& options
);

/// @brief Returns Json-string representation of protobuf message
/// @throws formats::json::Exception
std::string ToJsonString(const google::protobuf::Message& message);

/// @brief Returns Json-string representation of protobuf message
/// @throws formats::json::Exception
std::string ToJsonString(
    const google::protobuf::Message& message,
    const google::protobuf::util::JsonPrintOptions& options
);

/// @brief Parses Json to a protobuf message. Throws on unknown enum values and unknown fields by default.
/// @throws formats::json::Exception on field type mismatch, unknown enum values and unknown fields.
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::JsonToMessage .
template <typename Message>
Message JsonToMessage(const formats::json::Value& json) {
    Message message;
    impl::FromJsonStringImpl(formats::json::ToString(json), message, impl::kDefaultJsonParseOptions);
    return message;
}

/// @brief Parses Json to a protobuf message. Throws on unknown enum values.
/// @throws formats::json::Exception on field type mismatch and unknown enum values.
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::JsonToMessage .
template <typename Message>
Message JsonToMessage(const formats::json::Value& json, const google::protobuf::util::JsonParseOptions& options) {
    Message message;
    impl::FromJsonStringImpl(formats::json::ToString(json), message, options);
    return message;
}

/// @brief Parses Json to a protobuf message. Throws on unknown enum values and unknown fields by default.
/// @throws formats::json::Exception on invalid json, field type mismatch, unknown enum values and unknown fields.
template <typename Message>
Message FromJsonString(std::string_view json_string) {
    Message message;
    impl::FromJsonStringImpl(json_string, message, impl::kDefaultJsonParseOptions);
    return message;
}

/// @brief Parses Json to a protobuf message. Throws on unknown enum values.
/// @throws formats::json::Exception on invalid json, field type mismatch and unknown enum values.
template <typename Message>
Message FromJsonString(std::string_view json_string, const google::protobuf::util::JsonParseOptions& options) {
    Message message;
    impl::FromJsonStringImpl(json_string, message, options);
    return message;
}

}  // namespace ugrpc

namespace formats::serialize {

/// @brief Conversion from any `google::protobuf::Message` to @ref formats::json::Value.
/// Uses the same format as @ref ugrpc::MessageToJson with its default options.
///
/// Works for `google::protobuf::Value`, `google::protobuf::Struct`, `google::protobuf::ListValue`
/// (top-level and nested) as well, converts them without extra objects in JSON representation.
///
/// Use as:
/// @code{.cpp}
/// auto json = formats::json::ValueBuilder{message}.ExtractValue();
/// @endcode
json::Value Serialize(const google::protobuf::Message& message, To<json::Value>);

}  // namespace formats::serialize

namespace formats::parse {

/// @brief Conversion from @ref formats::json::Value to `google::protobuf::Message`.
/// Uses the same format as @ref ugrpc::JsonToMessage with its default options.
///
/// Works for `google::protobuf::Value`, `google::protobuf::Struct`, `google::protobuf::ListValue`
/// (top-level and nested) as well, converts them without extra objects in JSON representation.
///
/// Use as:
/// @code{.cpp}
/// auto value = json.As<google::protobuf::Value>();
/// @endcode
template <typename Message, typename = std::enable_if_t<std::is_base_of_v<google::protobuf::Message, Message>>>
Message Parse(const json::Value& value, To<Message>) {
    return ugrpc::JsonToMessage<Message>(value);
}

/// @cond
// Implementation detail: optimization for `google::protobuf::Value` specifically.
google::protobuf::Value Parse(const json::Value& value, To<google::protobuf::Value>);

// Implementation detail: optimization for `google::protobuf::Struct` specifically.
google::protobuf::Struct Parse(const json::Value& value, To<google::protobuf::Struct>);

// Implementation detail: optimization for `google::protobuf::ListValue` specifically.
google::protobuf::ListValue Parse(const json::Value& value, To<google::protobuf::ListValue>);
/// @endcond

}  // namespace formats::parse

USERVER_NAMESPACE_END
