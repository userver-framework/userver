#include <algorithm>
#include <storages/postgres/connlimit_watchdog.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {
constexpr CommandControl kCommandControl{std::chrono::seconds(2), std::chrono::seconds(2)};
constexpr std::size_t kMinReservedConnections = 2;
constexpr std::size_t kMaxReservedConnections = 10;
constexpr double kReservedConnectionsPercentage = 0.05;

constexpr int kMaxStepsWithError = 3;

std::size_t GetMaxConnections(Transaction& trx) {
    const auto max_server_connections = USERVER_NAMESPACE::utils::FromString<
        ssize_t>(trx.Execute("SHOW max_connections;").AsSingleRow<std::string>());
    auto max_user_connections =
        trx.Execute("SELECT rolconnlimit FROM pg_roles WHERE rolname = current_user").AsSingleRow<ssize_t>();
    if (max_user_connections < 0) {
        max_user_connections = max_server_connections;
    }
    std::size_t max_connections = std::min(max_server_connections, max_user_connections);

    const auto reserved_connections = std::max(
        kMinReservedConnections,
        std::min(kMaxReservedConnections, static_cast<std::size_t>(max_connections * kReservedConnectionsPercentage))
    );
    if (max_connections > reserved_connections) {
        max_connections -= reserved_connections;
    } else {
        max_connections = 1;
    }
    return max_connections;
}

}  // namespace

ConnlimitWatchdog::ConnlimitWatchdog(
    detail::ClusterImpl& cluster,
    testsuite::TestsuiteTasks& testsuite_tasks,
    int shard_number,
    std::size_t min_fallback_connections,
    std::function<void()> on_new_connlimit,
    std::size_t non_pool_connections_per_instance,
    std::string host_name
)
    : cluster_(cluster),
      connlimit_(0),
      on_new_connlimit_(std::move(on_new_connlimit)),
      testsuite_tasks_(testsuite_tasks),
      shard_number_(shard_number),
      min_fallback_connections_(std::max(min_fallback_connections, kDefaultPoolMinSize)),
      non_pool_connections_per_instance_(non_pool_connections_per_instance),
      host_name_(std::move(host_name))
{}

void ConnlimitWatchdog::TrySetupTable() {
    auto trx = BeginTransaction();
    trx.Execute(R"(
      CREATE TABLE IF NOT EXISTS u_clients (
          hostname TEXT PRIMARY KEY,
          updated TIMESTAMPTZ NOT NULL,
          max_connections INTEGER NOT NULL
      );
    )");
    trx.Execute("ALTER TABLE u_clients ADD COLUMN IF NOT EXISTS cur_user TEXT");
    trx.Execute("ALTER TABLE u_clients SET UNLOGGED");
    trx.Commit();
    table_is_ready_ = true;
}

void ConnlimitWatchdog::Start() {
    try {
        TrySetupTable();
    } catch (const storages::postgres::AccessRuleViolation& e) {
        // Possible in some CREATE TABLE IF NOT EXISTS races with other services
        table_is_ready_ = true;
        LOG_WARNING() << "Table already exists (not a fatal error): " << e;
    } catch (const storages::postgres::UniqueViolation& e) {
        // Possible in some CREATE TABLE IF NOT EXISTS races with other services
        table_is_ready_ = true;
        LOG_WARNING() << "Table already exists (not a fatal error): " << e;
    } catch (const storages::postgres::ClusterUnavailable& e) {
        if (!cluster_.HasAliveHosts()) {
            throw;
        }
        LOG_WARNING() << "Can't create u_clients, there is no writable master: will try later..." << e;
    }

    if (testsuite_tasks_.IsEnabled()) {
        testsuite_tasks_
            .RegisterTask(fmt::format("connlimit_watchdog_{}_{}", cluster_.GetDbName(), shard_number_), [this] {
                StepV1();
            });
    } else {
        StepV2();
        periodic_.Start("connlimit_watchdog", {std::chrono::seconds(2)}, [this] { StepV2(); });
    }
}

void ConnlimitWatchdog::StepV1() {
    // Beware! Do **not** change queries in StepV*, but rather provide a new StepV* to avoid migration issues.
    static const Query kUpsertClientMaxConnections{
        R"(
        INSERT INTO u_clients (hostname, updated, max_connections)
        VALUES ($1, NOW(), $2)
        ON CONFLICT (hostname) DO UPDATE SET updated = NOW(), max_connections = $2
    )",
        "UpsertMaxConnectionsV1"
    };

    static const Query kSelectInstances{
        R"(
        SELECT count(*) FROM u_clients WHERE updated >= NOW() - make_interval(secs => 15)
    )",
        "SelectInstancesV1"
    };

    static auto hostname = hostinfo::blocking::GetRealHostName();

    DoStep(hostname, kUpsertClientMaxConnections, kSelectInstances);
}

void ConnlimitWatchdog::StepV2() {
    // Beware! Do **not** change queries in StepV*, but rather provide a new StepV* to avoid migration issues.
    static const Query kUpsertClientMaxConnections{
        R"(
              INSERT INTO u_clients (hostname, updated, max_connections, cur_user)
              VALUES ($1, NOW(), $2, current_user)
              ON CONFLICT (hostname) DO UPDATE SET updated = NOW(), max_connections = $2, cur_user = current_user
            )",
        "UpsertMaxConnectionsV2"
    };

    static const Query kSelectInstances{
        R"(
            SELECT count(*) FROM u_clients WHERE updated >= NOW() - make_interval(secs => 15) AND (cur_user = current_user OR cur_user is NULL)
            )",
        "SelectInstancesV2"
    };

    DoStep(host_name_, kUpsertClientMaxConnections, kSelectInstances);
}

void ConnlimitWatchdog::Stop() { periodic_.Stop(); }

std::size_t ConnlimitWatchdog::GetConnlimit() const noexcept { return connlimit_.load(); }

void ConnlimitWatchdog::DoStep(
    std::string_view hostname,
    const Query& update_max_connections_query,
    const Query& select_instances_query
) {
    try {
        if (!table_is_ready_) {
            TrySetupTable();
        }

        auto trx = BeginTransaction();

        const auto max_connections = GetMaxConnections(trx);

        trx.Execute(update_max_connections_query, hostname, static_cast<int>(GetConnlimit()));
        auto instances = trx.Execute(select_instances_query).AsSingleRow<int>();

        UpdateConnectionsLimit(max_connections, instances);

        trx.Commit();
        steps_with_errors_ = 0;
    } catch (const ClusterUnavailable& e) {
        if (cluster_.HasAliveHosts()) {
            KeepConnlimitOnUnwritableMaster(e);
        } else {
            ReduceConnlimitOnError(e);
        }
    } catch (const Error& e) {
        ReduceConnlimitOnError(e);
    }

    on_new_connlimit_();
}

void ConnlimitWatchdog::ReduceConnlimitOnError(const Error& e) {
    if (++steps_with_errors_ > kMaxStepsWithError) {
        /*
         * Something's wrong with PG server. Try to lower the load by lowering
         * max connection to a small value. Active connections will be gracefully
         * closed. When the server returns the response, we'll get the real
         * connlimit value. The period with "too low max_connections" should be
         * relatively small.
         */
        const auto previous_connlimit = connlimit_.load();
        connlimit_ = std::max(previous_connlimit / 2, min_fallback_connections_);
        steps_with_errors_ = 0;
    }
    LOG_WARNING() << fmt::format("Can't connect to u_clients. Fallback max_size to {}. ", connlimit_.load()) << e;
}

void ConnlimitWatchdog::KeepConnlimitOnUnwritableMaster(const Error& e) {
    if (connlimit_.load() == 0) {
        connlimit_ = min_fallback_connections_;
    }
    LOG_LIMITED_WARNING(
        "Can't write to u_clients, the master does not accept writes while the cluster is "
        "alive. Keeping max_size at {}. ",
        connlimit_.load()
    ) << e;
}

Transaction ConnlimitWatchdog::BeginTransaction() {
    return cluster_.Begin({ClusterHostType::kMaster}, {}, kCommandControl);
}

void ConnlimitWatchdog::UpdateConnectionsLimit(std::size_t max_connections, std::size_t instances) {
    if (instances == 0) {
        instances = 1;
    }

    auto new_connlimit = max_connections / instances;
    if (new_connlimit > non_pool_connections_per_instance_) {
        new_connlimit -= non_pool_connections_per_instance_;
    } else {
        // ConnectionPool does not support max_size=0. Keep one pool connection
        // even when the server limit is insufficient for all non-pool connections.
        new_connlimit = 1;
    }
    auto previous_connlimit = connlimit_.exchange(new_connlimit);
    LOG((previous_connlimit == new_connlimit) ? logging::Level::kDebug : logging::Level::kWarning
    ) << "max_connections = "
      << max_connections << ", instances = " << instances << ", non_pool_connections_per_instance = "
      << non_pool_connections_per_instance_ << ", connlimit = " << new_connlimit;
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
