#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

bool IsMessage(const google::protobuf::FieldDescriptor& field);

bool HasSecrets(const google::protobuf::Message& message);

/// @warning This causes a segmentation fault for messages containing optional fields in protobuf versions prior to 3.13
/// See https://github.com/protocolbuffers/protobuf/issues/7801
void TrimSecrets(google::protobuf::Message& message);

std::string ToLimitedString(const google::protobuf::Message& message, std::size_t limit);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
