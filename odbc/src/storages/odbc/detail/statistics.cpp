#include <storages/odbc/detail/statistics.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

void DumpMetric(utils::statistics::Writer& writer, const InstanceStatistics& stats) {
    if (auto conn = writer["connections"]) {
        conn["opened"] = stats.connection.open_total;
        conn["closed"] = stats.connection.drop_total;
        conn["active"] = stats.connection.active;
        conn["busy"] = stats.connection.used;
        conn["max"] = stats.connection.maximum;
        conn["waiting"] = stats.connection.waiting;
    }

    if (auto trx = writer["transactions"]) {
        trx["total"] = stats.transaction.total;
        trx["committed"] = stats.transaction.commit_total;
        trx["rolled-back"] = stats.transaction.rollback_total;
        trx["no-tran"] = stats.transaction.out_of_trx_total;

        auto timing = trx["timings"];
        timing["full"] = stats.transaction.total_percentile;
        timing["busy"] = stats.transaction.busy_percentile;
        timing["acquire-connection"] = stats.acquire_percentile;
        timing["connecting"] = stats.connection_percentile;
    }

    if (auto query = writer["queries"]) {
        query["executed"] = stats.transaction.execute_total;
    }

    if (auto errors = writer["errors"]) {
        constexpr std::string_view kOdbcError = "odbc_error";
        errors.ValueWithLabels(stats.transaction.error_execute_total, {kOdbcError, "query-exec"});
        errors.ValueWithLabels(stats.transaction.execute_timeout, {kOdbcError, "query-timeout"});
        errors.ValueWithLabels(stats.connection.error_total, {kOdbcError, "connection"});
        errors.ValueWithLabels(stats.pool_exhaust_errors, {kOdbcError, "pool"});
        errors.ValueWithLabels(stats.connection.error_timeout, {kOdbcError, "connection-timeout"});
    }
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
