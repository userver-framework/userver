#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

extern const dynamic_config::Key<bool> kDeadlinePropagationEnabled;

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
