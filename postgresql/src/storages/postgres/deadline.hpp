#pragma once

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @throws ConnectionInterrupted if deadline is expired.
void CheckDeadlineIsExpired(const dynamic_config::Snapshot&);

TimeoutDuration AdjustTimeout(TimeoutDuration timeout, bool& adjusted);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
