#include <userver/server/request/impl/client_metrics_shard.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request::impl {

engine::TaskInheritedVariable<ClientMetricsShard> kClientMetricsShard;

}  // namespace server::request::impl

USERVER_NAMESPACE_END
