#include <storages/mongo/congestion_control/sensor.hpp>

#include <algorithm>  // for std::max

#include <storages/mongo/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::cc {

namespace {

AccumulatedDataByCollection SumStats(const stats::PoolStatistics& stats) {
    AccumulatedDataByCollection result;
    for (const auto& [coll, coll_stats] : stats.collections) {
        for (const auto& [op, op_stats] : coll_stats->items) {
            result[coll].total_queries += op_stats->GetTotalQueries().value;
            /*
             * Ignore kPoolOverload - CC leads to a shrank pool and burst of such
             * errors, it is not an explicit sign of bad mongo performance;
             * Ignore kNetwork - Deadline Propagation or simple timeouts might be the
             * reason.
             */
            result[coll].timeouts += op_stats->GetCounter(stats::ErrorType::kClusterUnavailable).value;
            result[coll].timings_sum += op_stats->timings_sum.Load().value;
        }
    }
    return result;
}

std::unordered_map<std::string, Sensor::SingleObjectData> ConvertToSensorData(const AccumulatedDataByCollection& data) {
    std::unordered_map<std::string, Sensor::SingleObjectData> result;
    for (const auto& [coll, coll_data] : data) {
        const auto total = std::max(coll_data.total_queries, std::uint64_t{1});
        LOG_TRACE() << "total queries = " << total;

        LOG_TRACE() << "timings sum = " << coll_data.timings_sum << "ms";

        const auto timeout_rate = static_cast<double>(coll_data.timeouts) / total;
        LOG_TRACE() << "timeout rate = " << timeout_rate;
        result[coll] = {total, coll_data.timeouts, coll_data.timings_sum};
    }
    return result;
}

}  // namespace

bool operator<(const AccumulatedData& lhs, const AccumulatedData& rhs) noexcept {
    return lhs.total_queries < rhs.total_queries && lhs.timeouts < rhs.timeouts && lhs.timings_sum < rhs.timings_sum;
}

AccumulatedData operator-(const AccumulatedData& lhs, const AccumulatedData& rhs) noexcept {
    AccumulatedData result;
    result.total_queries = lhs.total_queries - rhs.total_queries;
    result.timeouts = lhs.timeouts - rhs.timeouts;
    result.timings_sum = lhs.timings_sum - rhs.timings_sum;
    return result;
}

AccumulatedDataByCollection
operator-(const AccumulatedDataByCollection& lhs, const AccumulatedDataByCollection& rhs) noexcept {
    AccumulatedDataByCollection result;
    for (const auto& [coll, data] : lhs) {
        if (rhs.count(coll) > 0) {
            if (data < rhs.at(coll)) {
                LOG_WARNING("Current stats of collection '{}' is less than previous ones", coll);
                continue;
            } else {
                result[coll] = data - rhs.at(coll);
            }
        } else {
            result[coll] = data;
        }
    }
    return result;
}

Sensor::Sensor(impl::PoolImpl& pool) : pool_(pool) {}

Sensor::Data Sensor::GetCurrent() {
    const auto& stats = pool_.GetStatistics();
    const auto new_data = SumStats(stats);
    const auto last_data = std::exchange(last_data_by_collection_, new_data);

    const auto diff = new_data - last_data;

    const auto data = ConvertToSensorData(diff);

    const auto current_load = pool_.SizeApprox();
    return {data, current_load};
}

}  // namespace storages::mongo::cc

USERVER_NAMESPACE_END
