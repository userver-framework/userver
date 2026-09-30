#pragma once

/// @file userver/ugrpc/proto_json.hpp
/// @brief Utilities for conversion Protobuf -> Json
/// @ingroup userver_formats_serialize userver_formats_parse

#include <string>
#include <string_view>

#include <google/protobuf/message.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>  // kept: legacy users rely on this transitive include
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

namespace impl {

extern const google::protobuf::util::JsonPrintOptions kDefaultJsonPrintOptions;

extern const google::protobuf::util::JsonParseOptions kDefaultJsonParseOptions;

/// @throws formats::json::Exception on invalid JSON or conversion failure
void FromJsonStringImpl(
    std::string_view json_string,
    google::protobuf::Message& output,
    const google::protobuf::util::JsonParseOptions& options
);

/// @throws formats::json::Exception on conversion failure
void JsonToMessageImpl(
    const formats::json::Value& json,
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
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::MessageToJsonString .
std::string ToJsonString(const google::protobuf::Message& message);

/// @brief Returns Json-string representation of protobuf message
/// @throws formats::json::Exception
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::MessageToJsonString .
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
    impl::JsonToMessageImpl(json, message, impl::kDefaultJsonParseOptions);
    return message;
}

/// @brief Parses Json to a protobuf message. Throws on unknown enum values.
/// @throws formats::json::Exception on field type mismatch and unknown enum values.
/// @warning This function is **deprecated** and will be removed, use @ref protobuf::json::JsonToMessage .
template <typename Message>
Message JsonToMessage(const formats::json::Value& json, const google::protobuf::util::JsonParseOptions& options) {
    Message message;
    impl::JsonToMessageImpl(json, message, options);
    return message;
}

/// @brief Parses Json to a protobuf message. Throws on unknown enum values and unknown fields by default.
/// @throws formats::json::Exception on invalid json, field type mismatch, unknown enum values and unknown fields.
/// @warning This function is **deprecated** and will be removed, use @ref formats::json::FromString and
///          @ref protobuf::json::JsonToMessage .
template <typename Message>
Message FromJsonString(std::string_view json_string) {
    Message message;
    impl::FromJsonStringImpl(json_string, message, impl::kDefaultJsonParseOptions);
    return message;
}

/// @brief Parses Json to a protobuf message. Throws on unknown enum values.
/// @throws formats::json::Exception on invalid json, field type mismatch and unknown enum values.
/// @warning This function is **deprecated** and will be removed, use @ref formats::json::FromString and
///          @ref protobuf::json::JsonToMessage .
template <typename Message>
Message FromJsonString(std::string_view json_string, const google::protobuf::util::JsonParseOptions& options) {
    Message message;
    impl::FromJsonStringImpl(json_string, message, options);
    return message;
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
