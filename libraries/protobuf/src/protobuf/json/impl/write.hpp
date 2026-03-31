#pragma once

#include <google/protobuf/message.h>

#include <userver/formats/json/value_builder.hpp>
#include <userver/protobuf/json/convert_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

[[nodiscard]] formats::json::ValueBuilder WriteMessage(
    const ::google::protobuf::Message& message,
    const PrintOptions& options
);

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
