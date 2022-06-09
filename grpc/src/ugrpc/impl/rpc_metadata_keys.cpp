#include <ugrpc/impl/rpc_metadata_keys.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

const grpc::string kXYaTraceId = "x-yatraceid";
const grpc::string kXYaSpanId = "x-yaspanid";
const grpc::string kXYaRequestId = "x-yarequestid";

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
