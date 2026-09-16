#pragma once

#include <userver/storages/postgres/postgres_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @throws ConnectionInterrupted if deadline is expired.
void CheckDeadlineIsExpired();

TimeoutDuration AdjustTimeout(TimeoutDuration timeout, bool& adjusted);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
