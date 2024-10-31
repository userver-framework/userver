#pragma once

#include <cstddef>
#include <string>

namespace google::protobuf {

class Message;
class FieldDescriptor;

}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

bool IsMessage(const google::protobuf::FieldDescriptor& field);

/// @warning This causes a segmentation fault for messages containing optional fields in protobuf versions prior to 3.13
/// See https://github.com/protocolbuffers/protobuf/issues/7801
void TrimSecrets(google::protobuf::Message& message);

std::string ToString(const google::protobuf::Message& message, std::size_t max_msg_size);

std::string ToJsonString(const google::protobuf::Message& message, std::size_t max_msg_size);

// Same as ToString but also trims the secrets
///
/// @warning This causes a segmentation fault for messages containing optional fields in protobuf versions prior to 3.13
/// See https://github.com/protocolbuffers/protobuf/issues/7801
std::string ToLogString(const google::protobuf::Message& message, std::size_t max_msg_size);

// Same as ToJsonString but also trims the secrets
///
/// @warning This causes a segmentation fault for messages containing optional fields in protobuf versions prior to 3.13
/// See https://github.com/protocolbuffers/protobuf/issues/7801
std::string ToJsonLogString(const google::protobuf::Message& message, std::size_t max_msg_size);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
