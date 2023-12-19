#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

extern const dynamic_config::Key<bool> kServerCancelTaskByDeadline;

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
