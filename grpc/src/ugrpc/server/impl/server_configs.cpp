#include "server_configs.hpp"

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

const dynamic_config::Key<bool> kServerCancelTaskByDeadline{
    "USERVER_GRPC_SERVER_CANCEL_TASK_BY_DEADLINE", true};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
