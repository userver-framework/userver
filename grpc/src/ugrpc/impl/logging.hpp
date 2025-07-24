#pragma once

#include <string>

#include <google/protobuf/message.h>
#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

extern const std::string kBodyTag;
extern const std::string kCodeTag;
extern const std::string kComponentTag;
extern const std::string kMessageMarshalledLenTag;
extern const std::string kTypeTag;

std::string GetMessageForLogging(const google::protobuf::Message& message, std::size_t max_size);

std::string GetErrorDetailsForLogging(const grpc::Status& status);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
