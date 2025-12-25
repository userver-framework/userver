#include <userver/ydb/component.hpp>

#include <unordered_set>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/async.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ydb/coordination.hpp>
#include <userver/ydb/credentials.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/federated_topic.hpp>
#include <userver/ydb/table.hpp>
#include <userver/ydb/topic.hpp>

#include <ydb/impl/config.hpp>
#include <ydb/impl/driver.hpp>
#include <ydb/impl/secdist.hpp>
#include <ydb/impl/stats.hpp>

#include <dynamic_config/variables/YDB_RETRY_BUDGET.hpp>

// YDB headers leak `ARCADIA_ROOT` macro, so we use __has_include()
#if __has_include("generated/src/ydb/component.yaml.hpp")
#include "generated/src/ydb/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

std::unordered_set<std::string> GetDbNames(const components::ComponentConfig& config) {
    return utils::AsContainer<std::unordered_set<std::string>>(
        Items(config["databases"]) |
        // Caution: returning a reference to temporary here results in UB.
        boost::adaptors::transformed([](auto kv) { return std::move(kv.key); })
    );
}

}  // namespace

struct YdbComponent::DatabaseUtils final {
    static Database Make(
        const std::string& dbname,
        const yaml_config::YamlConfig& dbconfig,
        const impl::secdist::DatabaseSettings& dbsettings,
        std::shared_ptr<NYdb::ICredentialsProviderFactory> credentials_provider_factory,
        const OperationSettings& operation_settings,
        const dynamic_config::Source& config_source
    ) {
        const auto table_settings = impl::ParseTableSettings(dbconfig, dbsettings);
        const auto topic_settings = impl::TopicSettings{};
        const auto
            driver_settings = impl::ParseDriverSettings(dbconfig, dbsettings, std::move(credentials_provider_factory));

        auto driver = std::make_shared<impl::Driver>(dbname, driver_settings);

        auto table_client = std::make_shared<TableClient>(table_settings, operation_settings, config_source, driver);

        auto topic_client = std::make_shared<TopicClient>(driver, topic_settings);

        auto federated_topic_client = std::make_shared<FederatedTopicClient>(driver, topic_settings);

        auto coordination_client = std::make_shared<CoordinationClient>(driver);

        return Database{
            std::move(driver),
            std::move(table_client),
            std::move(topic_client),
            std::move(federated_topic_client),
            std::move(coordination_client)
        };
    }
};

YdbComponent::YdbComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : components::ComponentBase(config, context),
      config_(context.FindComponent<components::DynamicConfig>().GetSource())
{
    auto secdist_settings =
        context.FindComponent<components::Secdist>().Get().Get<impl::secdist::YdbSettings>().settings;
    auto config_source = context.FindComponent<components::DynamicConfig>().GetSource();

    const auto operation_settings = config["operation-settings"].As<OperationSettings>();

    const auto dbnames = GetDbNames(config);

    for (const auto& dbname : dbnames) {
        auto dbsettings = utils::FindOrDefault(secdist_settings, dbname);
        const auto& dbconfig = config["databases"][dbname];

        std::shared_ptr<NYdb::ICredentialsProviderFactory> credentials_provider_factory;
        if (const auto credentials_config = dbconfig["credentials"]; !credentials_config.IsMissing()) {
            const auto& credentials_provider_component = context.FindComponent<
                CredentialsProviderComponent>(config["credentials-provider"].As<std::string>());
            credentials_provider_factory =
                credentials_provider_component.CreateCredentialsProviderFactory(credentials_config);
        }

        databases_
            .emplace(dbname, engine::CriticalAsyncNoSpan(engine::current_task::GetBlockingTaskProcessor(), [&] {
                                 return DatabaseUtils::Make(
                                     dbname,
                                     dbconfig,
                                     dbsettings,
                                     credentials_provider_factory,
                                     operation_settings,
                                     config_source
                                 );
                             }).Get());

        if (dbconfig.HasMember("aliases")) {
            for (const auto& config_alias : dbconfig["aliases"]) {
                const std::string& dbname_alias = config_alias.As<std::string>();
                databases_.emplace(
                    dbname_alias,
                    DatabaseUtils::Make(
                        dbname_alias,
                        dbconfig,
                        dbsettings,
                        credentials_provider_factory,
                        operation_settings,
                        config_source
                    )
                );
            }
        }
    }

    auto& stats_storage = context.FindComponent<components::StatisticsStorage>().GetStorage();
    statistic_holder_ = stats_storage.RegisterWriter("ydb", [this](utils::statistics::Writer& writer) {
        WriteStatistics(writer);
    });

    config_subscription_ = config_.UpdateAndListen(this, "ydb", &YdbComponent::OnConfigUpdate);
}

YdbComponent::~YdbComponent() { statistic_holder_.Unregister(); }

const YdbComponent::Database& YdbComponent::FindDatabase(const std::string& dbname) const {
    auto it = databases_.find(dbname);
    if (it == databases_.end()) {
        throw UndefinedDatabaseError(fmt::format(
            "Undefined ydb database name: {}. Available databases: [{}]",
            dbname,
            fmt::join(databases_ | boost::adaptors::map_keys, ", ")
        ));
    }
    return it->second;
}

std::shared_ptr<TableClient> YdbComponent::GetTableClient(const std::string& dbname) const {
    return FindDatabase(dbname).table_client;
}

std::shared_ptr<TopicClient> YdbComponent::GetTopicClient(const std::string& dbname) const {
    return FindDatabase(dbname).topic_client;
}

std::shared_ptr<FederatedTopicClient> YdbComponent::GetFederatedTopicClient(const std::string& dbname) const {
    return FindDatabase(dbname).federated_topic_client;
}

std::shared_ptr<CoordinationClient> YdbComponent::GetCoordinationClient(const std::string& dbname) const {
    return FindDatabase(dbname).coordination_client;
}

const NYdb::TDriver& YdbComponent::GetNativeDriver(const std::string& dbname) const {
    return FindDatabase(dbname).driver->GetNativeDriver();
}

const std::string& YdbComponent::GetDatabasePath(const std::string& dbname) const {
    return FindDatabase(dbname).driver->GetDbPath();
}

void YdbComponent::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
    for (const auto& [dbname, settings] : cfg[::dynamic_config::YDB_RETRY_BUDGET].extra) {
        databases_[dbname].driver->GetRetryBudget().SetSettings({
            static_cast<float>(settings.max_tokens),
            static_cast<float>(settings.token_ratio),
            settings.enabled,
        });
    }
}

yaml_config::Schema YdbComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<components::ComponentBase>("src/ydb/component.yaml");
}

void YdbComponent::WriteStatistics(utils::statistics::Writer& writer) const {
    for (const auto& [dbname, database] : databases_) {
        writer.ValueWithLabels(*database.table_client, {"ydb_database", dbname});
        writer.ValueWithLabels(*database.driver, {"ydb_database", dbname});
    }
}

}  // namespace ydb

USERVER_NAMESPACE_END
