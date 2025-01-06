#include <ugrpc/impl/rpc_metadata.hpp>

#include <userver/hostinfo/blocking/get_hostname.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

const grpc::string kXYaTraceId = "x-yatraceid";
const grpc::string kXYaSpanId = "x-yaspanid";
const grpc::string kXYaRequestId = "x-yarequestid";

const grpc::string kXBaggage = "baggage";

const grpc::string kTraceParent = "traceparent";

const grpc::string kXYaTaxiRatelimitedBy = "x-yataxi-ratelimited-by";
const grpc::string kXYaTaxiRatelimitReason = "x-yataxi-ratelimit-reason";

const grpc::string kHostname = hostinfo::blocking::GetRealHostName();

const grpc::string kCongestionControlRatelimitReason = "congestion-control";

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
