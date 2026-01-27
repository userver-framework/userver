#pragma once

/// @file userver/storages/postgres/component.hpp
/// @brief @copybrief components::Postgres

#include <chrono>

#include <userver/components/component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <userver/storages/postgres/database.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief PosgreSQL client component
///
/// Provides access to a PostgreSQL cluster.
///
/// ## Dynamic options:
/// * @ref POSTGRES_DEFAULT_COMMAND_CONTROL
/// * @ref POSTGRES_HANDLERS_COMMAND_CONTROL
/// * @ref POSTGRES_QUERIES_COMMAND_CONTROL
/// * @ref POSTGRES_CONNECTION_POOL_SETTINGS
/// * @ref POSTGRES_TOPOLOGY_SETTINGS
/// * @ref POSTGRES_CONNECTION_SETTINGS
/// * @ref POSTGRES_STATEMENT_METRICS_SETTINGS
/// * @ref POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED
///
/// ## Static configuration example:
///
/// ```
///  # yaml
///  postgres-taxi:
///    dbalias: taxi
///    blocking_task_processor: task-processor-name
///    max_replication_lag: 60s
///    min_pool_size: 4
///    max_pool_size: 15
///    max_queue_size: 200
///    max_statement_metrics: 50
/// ```
/// You must specify either `dbalias` or `dbconnection`.
/// If the component is configured with an alias, it will lookup connection data
/// in Secdist.
///
/// It is a common practice to provide a database connection string via
/// environment variables. To retrieve a value from the environment use
/// `dbconnection#env: THE_ENV_VARIABLE_WITH_CONNECTION_STRING` as described
/// in yaml_config::YamlConfig.
///
/// You must specify `blocking_task_processor` as well.
///
/// `max_replication_lag` can be used to tune replication lag limit for replicas.
/// Once the replica lag exceeds this value it will be automatically disabled.
/// Note, however, that client-size lag detection is not precise in nature
/// and can only provide the precision of couple seconds.
///
/// ## Secdist format
///
/// A PosgreSQL alias in secdist is described as a JSON array of objects
/// containing a single cluster description. There are two formats of describing
/// a cluster, the first one assigns predefined roles to DSNs, the second one
/// is just a list of DSNs and the Postgres component takes care of discovering
/// the cluster's topology itself.
///
/// Note that if the `dbalias` option is provided and components::Secdist component has `update-period` other
/// than 0, then new connections are created or gracefully closed as the secdist configuration change to new value.
///
/// ### Predefined roles
///
/// In predefined roles format the component requires single-host connection
/// strings.
///
/// ```json
/// {
///   "shard_number" : 0,
///   "master": "host=localhost dbname=example",
///   "sync_slave": "host=localhost dbname=example",
///   "slaves": [
///     "host=localhost dbname=example"
///   ]
/// }
/// ```
///
/// The predefined roles format is deprecated and the support will be removed
/// soon.
///
/// ### Automatic discovery
///
/// In automatic discovery format the connection strings are any valid
/// PostgreSQL connection strings including multi-host ones with the exception
/// of `target_session_attrs` which will be ignored.
///
/// ```json
/// {
///   "shard_number" : 0,
///   "hosts": [
///     "host=host1,host2,host3 dbname=example",
///     "postgresql://host1:5432,host2:6432,host3:12000/example"
///   ]
/// }
/// ```
///
/// The `shard_number` parameter is required in both formats and must match the
/// index of cluster description object in the alias array.
///
/// Please see [PostgreSQL documentation](https://www.postgresql.org/docs/12/libpq-connect.html#LIBPQ-CONNSTRING)
/// on connection strings.
///
/// ## Static options of components::Postgres :
/// @include{doc} scripts/docs/en/components_schema/postgresql/src/storages/postgres/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class Postgres : public ComponentBase {
public:
    /// Default shard number
    static constexpr size_t kDefaultShardNumber = 0;
    /// Default command control
    static constexpr storages::postgres::CommandControl kDefaultCommandControl{
        std::chrono::milliseconds{500},  // network timeout
        std::chrono::milliseconds{250}   // statement timeout
    };

    /// Component constructor
    Postgres(const ComponentConfig&, const ComponentContext&);
    /// Component destructor
    ~Postgres() override;

    /// Cluster accessor for default shard number
    storages::postgres::ClusterPtr GetCluster() const;

    /// Cluster accessor for specific shard number
    storages::postgres::ClusterPtr GetClusterForShard(size_t shard) const;

    /// Get total shard count
    size_t GetShardCount() const;

    /// Get database object
    storages::postgres::DatabasePtr GetDatabase() const { return database_; }

    /// Reports statistics for PostgreSQL driver
    void ExtendStatistics(utils::statistics::Writer& writer);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

    void OnSecdistUpdate(const storages::secdist::SecdistConfig& secdist);

    std::string name_;
    std::string db_name_;
    std::string dbalias_;
    storages::postgres::ClusterSettings initial_settings_;
    storages::postgres::DatabasePtr database_;

    // Subscriptions must be the last fields, because the fields above are used
    // from callbacks.
    concurrent::AsyncEventSubscriberScope config_subscription_;
    concurrent::AsyncEventSubscriberScope secdist_subscription_;
    utils::statistics::Entry statistics_holder_;
    dynamic_config::Source config_source_;
};

template <>
inline constexpr bool kHasValidate<Postgres> = true;

}  // namespace components

USERVER_NAMESPACE_END
