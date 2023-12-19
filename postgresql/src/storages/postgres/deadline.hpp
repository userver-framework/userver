#pragma once

#include <userver/dynamic_config/snapshot.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @throws ConnectionInterrupted if deadline is expired.
void CheckDeadlineIsExpired(const dynamic_config::Snapshot&);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
