#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

extern const dynamic_config::Key<bool> kEnforceClientTaskDeadline;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
