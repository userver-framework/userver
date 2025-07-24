#include <userver/storages/postgres/component.hpp>

#include <optional>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <storages/postgres/postgres_secdist.hpp>
#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <storages/postgres/experiments.hpp>

#include <dynamic_config/variables/POSTGRES_CONNECTION_PIPELINE_EXPERIMENT.hpp>
#include <dynamic_config/variables/POSTGRES_OMIT_DESCRIBE_IN_EXECUTE.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
namespace {

constexpr auto kStatisticsName = "postgresql";

storages::postgres::ConnlimitMode ParseConnlimitMode(const std::string& value) {
    if (value == "manual") return storages::postgres::ConnlimitMode::kManual;
    if (value == "auto") return storages::postgres::ConnlimitMode::kAuto;

    UINVARIANT(false, "Unknown connlimit mode: " + value);
}

storages::postgres::OmitDescribeInExecuteMode ParseOmitDescribe(const dynamic_config::Snapshot& snapshot) {
    return snapshot[::dynamic_config::POSTGRES_OMIT_DESCRIBE_IN_EXECUTE] ==
                   storages::postgres::kOmitDescribeExperimentVersion
               ? storages::postgres::OmitDescribeInExecuteMode::kEnabled
               : storages::postgres::OmitDescribeInExecuteMode::kDisabled;
}

template <typename T>
void MergeField(T& field, const std::optional<T>& opt) {
    if (opt) field = *opt;
}

void MergePoolSettings(
    std::optional<storages::postgres::PoolSettingsDynamic>&& dynamic_settings_opt,
    storages::postgres::PoolSettings& static_settings
) {
    if (dynamic_settings_opt.has_value()) {
        auto& dynamic_settings = dynamic_settings_opt.value();
        MergeField(static_settings.max_size, dynamic_settings.max_size);
        MergeField(static_settings.min_size, dynamic_settings.min_size);
        MergeField(static_settings.max_queue_size, dynamic_settings.max_queue_size);
        MergeField(static_settings.connecting_limit, dynamic_settings.connecting_limit);
    }
}

void MergeConnectionSettings(
    std::optional<storages::postgres::ConnectionSettingsDynamic>&& dynamic_settings_opt,
    storages::postgres::ConnectionSettings& static_settings
) {
    if (dynamic_settings_opt.has_value()) {
        auto& dynamic_settings = dynamic_settings_opt.value();
        MergeField(static_settings.prepared_statements, dynamic_settings.prepared_statements);
        MergeField(static_settings.user_types, dynamic_settings.user_types);
        MergeField(static_settings.max_prepared_cache_size, dynamic_settings.max_prepared_cache_size);
        MergeField(static_settings.ignore_unused_query_params, dynamic_settings.ignore_unused_query_params);
        MergeField(static_settings.recent_errors_threshold, dynamic_settings.recent_errors_threshold);
        MergeField(static_settings.discard_on_connect, dynamic_settings.discard_on_connect);
        MergeField(static_settings.deadline_propagation_enabled, dynamic_settings.deadline_propagation_enabled);
        if (const auto max_ttl = dynamic_settings.max_ttl; max_ttl) {
            static_settings.max_ttl = *max_ttl;
        }
    }
}

}  // namespace

Postgres::Postgres(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      name_{config["name_alias"].As<std::string>(config.Name())},
      database_{std::make_shared<storages::postgres::Database>()},
      config_source_{context.FindComponent<DynamicConfig>().GetSource()} {
    storages::postgres::LogRegisteredTypesOnce();

    namespace pg = storages::postgres;

    const auto initial_config = config_source_.GetSnapshot();
    const auto& pg_config = initial_config[storages::postgres::kConfig];

    const auto dbalias = config["dbalias"].As<std::string>("");

    std::vector<pg::DsnList> cluster_desc;
    if (dbalias.empty()) {
        const pg::Dsn dsn{config["dbconnection"].As<std::string>()};
        const auto options = pg::OptionsFromDsn(dsn);
        db_name_ = options.dbname;
        cluster_desc.push_back(pg::SplitByHost(dsn));
    } else {
        try {
            auto& secdist = context.FindComponent<Secdist>();
            cluster_desc = secdist.Get().Get<pg::secdist::PostgresSettings>().GetShardedClusterDescription(dbalias);
            db_name_ = dbalias;
            dbalias_ = dbalias;
        } catch (const storages::secdist::SecdistError& ex) {
            LOG_ERROR() << "Failed to load Postgres config for dbalias " << dbalias << ": " << ex;
            throw;
        }
    }

    const auto monitoring_dbalias = config["monitoring-dbalias"].As<std::string>("");
    if (!monitoring_dbalias.empty()) {
        db_name_ = monitoring_dbalias;
    }

    initial_settings_.init_mode = config["sync-start"].As<bool>(true) ? storages::postgres::InitMode::kSync
                                                                      : storages::postgres::InitMode::kAsync;
    initial_settings_.db_name = db_name_;
    initial_settings_.connlimit_mode = ParseConnlimitMode(config["connlimit_mode"].As<std::string>("auto"));

    initial_settings_.topology_settings.max_replication_lag =
        config["max_replication_lag"].As<std::chrono::milliseconds>(storages::postgres::kDefaultMaxReplicationLag);

    initial_settings_.pool_settings = config.As<storages::postgres::PoolSettings>();
    MergePoolSettings(pg_config.pool_settings.GetOptional(name_), initial_settings_.pool_settings);

    initial_settings_.conn_settings = config.As<storages::postgres::ConnectionSettings>();
    MergeConnectionSettings(pg_config.connection_settings.GetOptional(name_), initial_settings_.conn_settings);

    initial_settings_.conn_settings.statement_log_mode =
        config["statement-log-mode"].As<storages::postgres::ConnectionSettings::StatementLogMode>();

    initial_settings_.conn_settings.pipeline_mode =
        initial_config[::dynamic_config::POSTGRES_CONNECTION_PIPELINE_EXPERIMENT] > 0
            ? storages::postgres::PipelineMode::kEnabled
            : storages::postgres::PipelineMode::kDisabled;
    initial_settings_.conn_settings.omit_describe_mode = ParseOmitDescribe(initial_config);
    initial_settings_.statement_metrics_settings = pg_config.statement_metrics_settings.GetOptional(name_).value_or(
        config.As<storages::postgres::StatementMetricsSettings>()
    );

    const auto task_processor_name = config["blocking_task_processor"].As<std::optional<std::string>>();
    auto& bg_task_processor = task_processor_name ? context.GetTaskProcessor(*task_processor_name)
                                                  : engine::current_task::GetBlockingTaskProcessor();

    error_injection::Settings ei_settings;
    auto ei_settings_opt = config["error-injection"].As<std::optional<error_injection::Settings>>();
    if (ei_settings_opt) ei_settings = *ei_settings_opt;

    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>().GetStorage();
    statistics_holder_ = statistics_storage.RegisterWriter(kStatisticsName, [this](utils::statistics::Writer& writer) {
        return ExtendStatistics(writer);
    });

    // Start all clusters here
    LOG_DEBUG() << "Start " << cluster_desc.size() << " shards for " << db_name_;

    const auto& testsuite_pg_ctl = context.FindComponent<components::TestsuiteSupport>().GetPostgresControl();
    auto& testsuite_tasks = testsuite::GetTestsuiteTasks(context);

    auto* resolver = clients::dns::GetResolverPtr(config, context);
    auto metrics = context.FindComponent<components::StatisticsStorage>().GetMetricsStorage();

    int shard_number = 0;
    for (auto& dsns : cluster_desc) {
        auto cluster = std::make_shared<pg::Cluster>(
            std::move(dsns),
            resolver,
            bg_task_processor,
            initial_settings_,
            storages::postgres::DefaultCommandControls{
                pg_config.default_command_control,
                pg_config.handlers_command_control,
                pg_config.queries_command_control},
            testsuite_pg_ctl,
            ei_settings,
            testsuite_tasks,
            config_source_,
            metrics,
            shard_number++
        );
        database_->clusters_.push_back(cluster);
    }

    config_subscription_ = config_source_.UpdateAndListen(this, "postgres", &Postgres::OnConfigUpdate);
    if (!dbalias_.empty()) {
        auto& secdist = context.FindComponent<Secdist>();
        secdist_subscription_ = secdist.GetStorage().UpdateAndListen(this, db_name_, &Postgres::OnSecdistUpdate);
    }

    LOG_DEBUG() << "Component ready";
}

Postgres::~Postgres() {
    statistics_holder_.Unregister();
    config_subscription_.Unsubscribe();
}

storages::postgres::ClusterPtr Postgres::GetCluster() const { return database_->GetCluster(); }

storages::postgres::ClusterPtr Postgres::GetClusterForShard(size_t shard) const {
    return database_->GetClusterForShard(shard);
}

size_t Postgres::GetShardCount() const { return database_->GetShardCount(); }

void Postgres::ExtendStatistics(utils::statistics::Writer& writer) {
    for (const auto& [i, cluster] : utils::enumerate(database_->clusters_)) {
        if (cluster) {
            const auto shard_name = "shard_" + std::to_string(i);
            writer.ValueWithLabels(
                *cluster->GetStatistics(),
                {{"postgresql_database", db_name_}, {"postgresql_database_shard", shard_name}}
            );
        }
    }
}

void Postgres::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
    const auto& pg_config = cfg[storages::postgres::kConfig];
    auto pool_settings = initial_settings_.pool_settings;
    MergePoolSettings(pg_config.pool_settings.GetOptional(name_), pool_settings);
    const auto topology_settings =
        pg_config.topology_settings.GetOptional(name_).value_or(initial_settings_.topology_settings);

    auto connection_settings = initial_settings_.conn_settings;
    MergeConnectionSettings(pg_config.connection_settings.GetOptional(name_), connection_settings);

    connection_settings.pipeline_mode = cfg[::dynamic_config::POSTGRES_CONNECTION_PIPELINE_EXPERIMENT] > 0
                                            ? storages::postgres::PipelineMode::kEnabled
                                            : storages::postgres::PipelineMode::kDisabled;
    connection_settings.omit_describe_mode = ParseOmitDescribe(cfg);
    const auto statement_metrics_settings =
        pg_config.statement_metrics_settings.GetOptional(name_).value_or(initial_settings_.statement_metrics_settings);

    for (const auto& cluster : database_->clusters_) {
        cluster->ApplyGlobalCommandControlUpdate(pg_config.default_command_control);
        cluster->SetHandlersCommandControl(pg_config.handlers_command_control);
        cluster->SetQueriesCommandControl(pg_config.queries_command_control);
        cluster->SetPoolSettings(pool_settings);
        cluster->SetTopologySettings(topology_settings);
        cluster->SetConnectionSettings(connection_settings);
        cluster->SetStatementMetricsSettings(statement_metrics_settings);
    }
}

void Postgres::OnSecdistUpdate(const storages::secdist::SecdistConfig& secdist) {
    const auto& cluster_desc =
        secdist.Get<storages::postgres::secdist::PostgresSettings>().GetShardedClusterDescription(dbalias_);
    database_->UpdateClusterDescription(cluster_desc);

    auto snapshot = config_source_.GetSnapshot();
    OnConfigUpdate(snapshot);
}

yaml_config::Schema Postgres::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: PosgreSQL client component
additionalProperties: false
properties:
    dbalias:
        type: string
        description: name of the database in secdist config (if available)
    name_alias:
        type: string
        description: name alias to use in configs (by default - component name)
    dbconnection:
        type: string
        description: connection DSN string (used if no dbalias specified)
    blocking_task_processor:
        type: string
        description: name of task processor for background blocking operations
        defaultDescription: engine::current_task::GetBlockingTaskProcessor()
    max_replication_lag:
        type: string
        description: replication lag limit for usable slaves
        defaultDescription: 60s
    min_pool_size:
        type: integer
        description: number of connections created initially
        defaultDescription: 4
    max_pool_size:
        type: integer
        description: limit of connections count
        defaultDescription: 15
    sync-start:
        type: boolean
        description: perform initial connections synchronously
        defaultDescription: false
    dns_resolver:
        type: string
        description: server hostname resolver type (getaddrinfo or async)
        defaultDescription: 'async'
        enum:
          - getaddrinfo
          - async
    persistent-prepared-statements:
        type: boolean
        description: cache prepared statements or not
        defaultDescription: true
    statement-log-mode:
        type: string
        enum:
          - hide
          - show
        description: whether to log SQL statements in a span tag
        defaultDescription: show
    user-types-enabled:
        type: boolean
        description: disabling will disallow use of user-defined types
        defaultDescription: true
    check-user-types:
        type: boolean
        description: |
            cancel service start if some user types have not been loaded, which
            helps to detect missing migrations
        defaultDescription: false
    ignore_unused_query_params:
        type: boolean
        description: disable check for not-NULL query params that are not used in query
        defaultDescription: false
    max-ttl-sec:
        type: integer
        minimum: 1
        description: the maximum lifetime for connections
    discard-all-on-connect:
        type: boolean
        description: execute discard all on new connections
        defaultDescription: true
    deadline-propagation-enabled:
        type: boolean
        description: whether statement timeout is affected by deadline propagation
        defaultDescription: true
    monitoring-dbalias:
        type: string
        description: name of the database for monitorings
        defaultDescription: calculated from dbalias or dbconnection options
    max_prepared_cache_size:
        type: integer
        description: prepared statements cache size limit
        defaultDescription: 200
    max_statement_metrics:
        type: integer
        description: limit of exported metrics for named statements
        defaultDescription: 0
    error-injection:
        type: object
        description: error-injection options
        additionalProperties: false
        properties:
            enabled:
                type: boolean
                description: enable error injection
                defaultDescription: false
            probability:
                type: number
                description: thrown exception probability
                defaultDescription: 0
            verdicts:
                type: array
                description: possible injection verdicts
                defaultDescription: empty
                items:
                    type: string
                    description: what error injection hook may decide to do
                    enum:
                      - timeout
                      - error
                      - max-delay
                      - random-delay
    max_queue_size:
        type: integer
        description: maximum number of clients waiting for a connection
        defaultDescription: 200
    pipeline_enabled:
        type: boolean
        description: turns on pipeline connection mode
        defaultDescription: false
    connecting_limit:
        type: integer
        description: limit for concurrent establishing connections number per pool (0 - unlimited)
        defaultDescription: 0
    connlimit_mode:
        type: string
        enum:
         - auto
         - manual
        description: how to learn the `max_pool_size`
)");
}

}  // namespace components

USERVER_NAMESPACE_END
