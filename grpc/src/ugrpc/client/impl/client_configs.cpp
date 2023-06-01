#include "client_configs.hpp"

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

const std::string kGrpcEnforceTaskDeadline =
    "USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION";

}  // namespace

bool ParseEnforceTaskDeadline(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get(kGrpcEnforceTaskDeadline).As<bool>();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
