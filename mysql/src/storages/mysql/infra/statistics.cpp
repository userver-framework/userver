#include <storages/mysql/infra/statistics.hpp>

#include <algorithm>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

void DumpMetric(utils::statistics::Writer& writer, const PoolConnectionStatistics& stats) {
    writer["overload"] = stats.overload;
    writer["created"] = stats.created;
    writer["closed"] = stats.closed;

    // created/closed are loaded independently, so a race may transiently make
    // closed > created; clamp to avoid reporting a negative "active" count.
    const auto active =
        static_cast<std::int64_t>(stats.created.Load().value) - static_cast<std::int64_t>(stats.closed.Load().value);
    writer["active"] = std::max(std::int64_t{0}, active);
    writer["busy"] = stats.acquired - stats.released;
}

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
