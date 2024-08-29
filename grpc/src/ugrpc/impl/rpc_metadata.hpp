#pragma once

#include <string>

#include <grpcpp/impl/codegen/config.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

extern const grpc::string kXYaTraceId;
extern const grpc::string kXYaSpanId;
extern const grpc::string kXYaRequestId;

extern const grpc::string kXBaggage;

extern const grpc::string kTraceParent;

extern const grpc::string kXYaTaxiRatelimitedBy;
extern const grpc::string kXYaTaxiRatelimitReason;

extern const grpc::string kHostname;

extern const grpc::string kCongestionControlRatelimitReason;

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
