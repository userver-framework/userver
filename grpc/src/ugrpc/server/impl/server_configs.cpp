#include "server_configs.hpp"

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

const std::string kGrpcServerCancelTaskByDeadline =
    "USERVER_GRPC_SERVER_CANCEL_TASK_BY_DEADLINE";

}  // namespace

bool ParseCancelTaskByDeadline(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get(kGrpcServerCancelTaskByDeadline).As<bool>();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
