#include "client_configs.hpp"

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

const dynamic_config::Key<bool> kEnforceClientTaskDeadline{
    "USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION", true};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
