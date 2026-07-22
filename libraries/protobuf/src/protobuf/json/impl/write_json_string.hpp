#pragma once

#include <string>

#include <google/protobuf/message.h>

#include <userver/protobuf/json/convert_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

[[nodiscard]] std::string WriteMessageToJsonString(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
);

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
