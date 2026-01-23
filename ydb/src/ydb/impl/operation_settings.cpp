#include <ydb/impl/operation_settings.hpp>

#include <algorithm>

#include <userver/ydb/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

std::chrono::milliseconds DeadlineToTimeout(engine::Deadline deadline) {
    const auto timeout = std::chrono::duration_cast<
        std::chrono::milliseconds>(deadline.IsReachable() ? deadline.TimeLeft() : engine::Deadline::Duration::max());
    if (timeout <= std::chrono::milliseconds::zero()) {
        throw DeadlineExceededError("deadline exceeded before the query");
    }
    return timeout;
}

NYdb::NQuery::EStatsMode ConvertStatsMode(NYdb::NTable::ECollectQueryStatsMode collect_query_stats_mode) {
    switch (collect_query_stats_mode) {
        case NYdb::NTable::ECollectQueryStatsMode::None:
            return NYdb::NQuery::EStatsMode::None;
        case NYdb::NTable::ECollectQueryStatsMode::Basic:
            return NYdb::NQuery::EStatsMode::Basic;
        case NYdb::NTable::ECollectQueryStatsMode::Full:
            return NYdb::NQuery::EStatsMode::Full;
        case NYdb::NTable::ECollectQueryStatsMode::Profile:
            return NYdb::NQuery::EStatsMode::Profile;
    }
}

}  // namespace

std::chrono::milliseconds GetBoundTimeout(std::chrono::milliseconds timeout, engine::Deadline deadline) {
    const auto max_timeout = impl::DeadlineToTimeout(deadline);
    return (std::chrono::milliseconds::zero() < timeout) ? std::min(timeout, max_timeout) : max_timeout;
}

NYdb::NQuery::TExecuteQuerySettings ToExecuteQuerySettings(const QuerySettings& query_settings) {
    NYdb::NQuery::TExecuteQuerySettings execute_query_settings;

    // Query Client doesn't have KeepInQueryCache, it caches automatically

    if (query_settings.collect_query_stats.has_value()) {
        execute_query_settings.StatsMode(ConvertStatsMode(*query_settings.collect_query_stats));
    }

    return execute_query_settings;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
