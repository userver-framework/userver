#pragma once

/// @file userver/proto-structs/json.hpp
/// @brief Utilities for converting protobuf structs to/from @ref formats::json::Value

#include <userver/formats/json/serialize.hpp>
#include <userver/proto-structs/convert.hpp>
#include <userver/proto-structs/type_mapping.hpp>
#include <userver/protobuf/json/convert.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief Converts protobuf struct @a obj to @ref formats::json::Value.
/// Conversion is performed by converting the protobuf struct to an intermediate protobuf message and then
/// converting the message to @ref formats::json::Value.
/// @tparam TStruct protobuf struct type
/// @throws WriteError if struct to message conversion has failed.
/// @throws PrintError if message to JSON conversion has failed.
template <typename TStruct>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] formats::json::Value StructToJson(TStruct&& obj, const protobuf::json::PrintOptions& options) {
    return protobuf::json::MessageToJson(StructToMessage(std::forward<TStruct>(obj)), options);
}

/// @brief Converts protobuf struct @a obj to an std::string, containing the JSON representation of the struct.
/// Conversion is performed by converting the protobuf struct to an intermediate protobuf message, then
/// converting the message to @ref formats::json::Value, and finally converting the value to a string.
/// @tparam TStruct protobuf struct type
/// @throws WriteError if struct to message conversion has failed.
/// @throws PrintError if message to JSON conversion has failed.
template <typename TStruct>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] std::string StructToJsonString(TStruct&& obj, const protobuf::json::PrintOptions& options) {
    return formats::json::ToString(StructToJson(std::forward<TStruct>(obj), options));
}

/// @brief Converts @a json to protobuf struct of type `TStruct`.
/// Conversion is performed by converting the @ref formats::json::Value to an intermediate protobuf
/// message and then converting the message to a protobuf struct.
/// @tparam TStruct protobuf struct type
/// @throws ParseError if JSON to message conversion has failed.
/// @throws MemberMissingException is @a json holds nothing.
/// @throws ReadError if message to struct conversion has failed.
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
template <typename TStruct>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] TStruct JsonToStruct(const formats::json::Value& json, const protobuf::json::ParseOptions& options) {
    using Message = traits::CompatibleMessageType<std::remove_cvref_t<TStruct>>;
    const auto message = protobuf::json::JsonToMessage<Message>(json, options);
    return MessageToStruct<TStruct>(message);
}

/// @brief Converts @a json_string to protobuf struct of type `TStruct`.
/// Conversion is performed by first converting the string to @ref formats::json::Value,
/// then converting the value to an intermediate protobuf message,
/// and finally converting the message to a protobuf struct.
/// @tparam TStruct protobuf struct type
/// @throws ParseError if JSON to message conversion has failed.
/// @throws MemberMissingException is @a json holds nothing.
/// @throws ReadError if message to struct conversion has failed.
/// The conversion is perfomed according to [ProtoJSON](https://protobuf.dev/programming-guides/json/) specification.
/// @warning Most of the legacy ProtoJSON behavior introduced for compatability with non-conformant implementations
///          is not supported. This behavior may be disabled in the future versions of the protobuf library thus
///          should not be relied upon.
/// @warning The `proto2` syntax is not fully supported and tested (at least extension fields are not supported).
template <typename TStruct>
requires traits::ProtoStruct<std::remove_cvref_t<TStruct>>
[[nodiscard]] TStruct JsonStringToStruct(std::string_view json_string, const protobuf::json::ParseOptions& options) {
    return JsonToStruct<TStruct>(formats::json::FromString(json_string), options);
}

}  // namespace proto_structs

USERVER_NAMESPACE_END
