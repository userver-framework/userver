#pragma once

#include <cstddef>
#include <string>

#include <google/protobuf/message.h>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::impl {

[[nodiscard]] std::string WriteMessageToDebugString(const ::google::protobuf::Message& message, std::size_t limit);

}  // namespace protobuf::json::impl

USERVER_NAMESPACE_END
