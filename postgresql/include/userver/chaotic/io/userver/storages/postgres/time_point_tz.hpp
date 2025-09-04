#pragma once

#include <userver/chaotic/convert/to.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

TimePointTz Convert(const std::string& str, chaotic::convert::To<TimePointTz>);

std::string Convert(const TimePointTz& tp, chaotic::convert::To<std::string>);

}  // namespace storages::postgres

USERVER_NAMESPACE_END
