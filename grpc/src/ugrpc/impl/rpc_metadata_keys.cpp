#include <ugrpc/impl/rpc_metadata_keys.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

const std::string kXYaTraceId = "x-yatraceid";
const std::string kXYaSpanId = "x-yaspanid";
const std::string kXYaRequestId = "x-yarequestid";

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
