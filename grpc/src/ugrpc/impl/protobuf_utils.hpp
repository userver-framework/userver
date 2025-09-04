#pragma once

#include <google/protobuf/message.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

std::string ToLimitedDebugString(const google::protobuf::Message& message, std::size_t limit);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
