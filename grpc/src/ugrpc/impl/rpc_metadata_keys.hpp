#pragma once

#include <string>

#include <grpcpp/impl/codegen/config.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

extern const grpc::string kXYaTraceId;
extern const grpc::string kXYaSpanId;
extern const grpc::string kXYaRequestId;
extern const grpc::string kXBaggage;

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
