#include <userver/chaotic/io/userver/storages/postgres/time_point_tz.hpp>
#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

TimePointTz Convert(const std::string& str, chaotic::convert::To<TimePointTz>) {
    return USERVER_NAMESPACE::storages::postgres::TimePointTz{USERVER_NAMESPACE::utils::datetime::Stringtime(str)};
}

std::string Convert(const TimePointTz& tp, chaotic::convert::To<std::string>) {
    return USERVER_NAMESPACE::utils::datetime::Timestring(tp.GetUnderlying());
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
