#pragma once

#include <google/protobuf/message.h>

#include <userver/formats/json/value.hpp>
#include <userver/protobuf/json/convert_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

void ReadMessage(const formats::json::Value& json, const ReadOptions& options, ::google::protobuf::Message& message);

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
