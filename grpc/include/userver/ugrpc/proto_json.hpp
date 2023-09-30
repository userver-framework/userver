#pragma once

/// @file userver/ugrpc/proto_json.hpp
/// @brief Utilities for conversion Protobuf -> Json

#include <google/protobuf/util/json_util.h>

#include <userver/formats/json.hpp>
#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// @brief Returns formats::json::Value representation of protobuf message
/// @throws SerializationError
formats::json::Value MessageToJson(const google::protobuf::Message& message);

/// @brief Converts message to human readable string
std::string ToString(const google::protobuf::Message& message);

/// @brief Returns Json-string representation of protobuf message
/// @throws formats::json::ConversionException
std::string ToJsonString(const google::protobuf::Message& message);

}  // namespace ugrpc

namespace formats::serialize {

json::Value Serialize(const google::protobuf::Message& message,
                      To<json::Value>);

}  // namespace formats::serialize

USERVER_NAMESPACE_END
