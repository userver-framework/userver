#pragma once

#include <atomic>
#include <cstddef>

#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace detail {
class ClusterImpl;
}

class ConnlimitWatchdog final {
public:
    ConnlimitWatchdog(
        detail::ClusterImpl& cluster,
        testsuite::TestsuiteTasks& testsuite_tasks,
        int shard_number,
        std::size_t min_fallback_connections,
        std::function<void()> on_new_connlimit,
        std::string host_name = hostinfo::blocking::GetRealHostName()
    );

    void Start();

    void Stop();

    // Beware! Do **not** change queries in StepV*, but rather provide a new StepV* to avoid migration issues.
    void StepV1();
    void StepV2();

    std::size_t GetConnlimit() const noexcept;

private:
    Transaction BeginTransaction();

    void UpdateConnectionsLimit(std::size_t max_connections, std::size_t instances);

    void DoStep(
        const std::string& hostname,
        const Query& update_max_connections_query,
        const Query& select_instances_query
    );

    detail::ClusterImpl& cluster_;
    std::atomic<std::size_t> connlimit_;
    std::function<void()> on_new_connlimit_;
    testsuite::TestsuiteTasks& testsuite_tasks_;
    int steps_with_errors_{0};
    USERVER_NAMESPACE::utils::PeriodicTask periodic_;
    int shard_number_;
    std::size_t min_fallback_connections_;
    std::string host_name_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
