#pragma once

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

using SteadyClock = USERVER_NAMESPACE::utils::datetime::SteadyClock;

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
