#include <storages/postgres/congestion_control/sensor.hpp>

#include <storages/postgres/detail/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::cc {

Sensor::Sensor(detail::ConnectionPool& pool)
    : pool_(pool)
{}

Sensor::Data Sensor::GetCurrent() {
    const auto& stats = pool_.GetStatistics();
    const std::uint64_t execute_timeout = stats.transaction.execute_timeout.Load().value;
    const std::uint64_t connection_timeout = stats.connection.error_timeout.Load().value;
    const std::uint64_t new_timeouts = execute_timeout + connection_timeout;
    const auto diff_timeouts = new_timeouts - last_timeouted_queries_;
    last_timeouted_queries_ = new_timeouts;

    const std::uint64_t new_total = stats.transaction.total.Load().value;
    auto diff_total = new_total - last_total_queries_;
    last_total_queries_ = new_total;
    if (diff_total == 0) {
        diff_total = 1;
    }

    const auto timeout_rate = static_cast<double>(diff_timeouts) / diff_total;
    LOG_DEBUG() << "timeout rate = " << timeout_rate;

    auto current_limit = stats.connection.maximum;
    return {{{Sensor::SingleObjectData::kCommonObjectName, {diff_total, diff_timeouts}}}, current_limit};
}

}  // namespace storages::postgres::cc

USERVER_NAMESPACE_END
