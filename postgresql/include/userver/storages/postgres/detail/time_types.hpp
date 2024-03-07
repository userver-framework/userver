#pragma once

#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

using SteadyClock = USERVER_NAMESPACE::utils::datetime::SteadyClock;
using SteadyCoarseClock = USERVER_NAMESPACE::utils::datetime::SteadyCoarseClock;

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
