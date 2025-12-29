#include <userver/storages/mongo/component.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <storages/mongo/mongo_secdist.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/mongo/component.yaml.hpp"        // Y_IGNORE
#include "generated/src/storages/mongo/component_multi.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
const std::string kStandardMongoPrefix = "mongo-";

auto ParsePoolConfig(const ComponentConfig& config) {
    auto pool_config = config.As<storages::mongo::PoolConfig>();
    pool_config.Validate(config.Name());
    return pool_config;
}

}  // namespace

Mongo::Mongo(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context)
{
    auto dbalias = config["dbalias"].As<std::string>("");

    std::string connection_string;
    storages::secdist::Secdist* secdist{};
    if (!dbalias.empty()) {
        dbalias_ = dbalias;
        secdist = &context.FindComponent<Secdist>().GetStorage();
        connection_string = storages::mongo::secdist::GetSecdistConnectionString(*secdist, dbalias);
    } else {
        connection_string = config["dbconnection"].As<std::string>();
    }

    auto* dns_resolver = clients::dns::GetResolverPtr(config, context);

    const auto pool_config = ParsePoolConfig(config);
    auto config_source = context.FindComponent<DynamicConfig>().GetSource();

    pool_ = std::make_shared<
        storages::mongo::Pool>(config.Name(), connection_string, pool_config, dns_resolver, config_source);

    if (!dbalias_.empty()) {
        secdist_subscriber_ = secdist->UpdateAndListen(this, dbalias_, &Mongo::OnSecdistUpdate);
    }

    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();

    auto section_name = config.Name();
    if (boost::algorithm::starts_with(section_name, kStandardMongoPrefix) &&
        section_name.size() != kStandardMongoPrefix.size())
    {
        section_name = section_name.substr(kStandardMongoPrefix.size());
    }
    statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
        "mongo",
        [this](utils::statistics::Writer& writer) {
            UASSERT(pool_);
            writer = *pool_;
        },
        {{"mongo_database", std::move(section_name)}}
    );
}

Mongo::~Mongo() {
    statistics_holder_.Unregister();
    secdist_subscriber_.Unsubscribe();
}

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

void Mongo::OnSecdistUpdate(const storages::secdist::SecdistConfig& config) {
    auto connection_string = storages::mongo::secdist::GetSecdistConnectionString(config, dbalias_);
    pool_->SetConnectionString(connection_string);
}

yaml_config::Schema Mongo::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<MultiMongo>("src/storages/mongo/component.yaml");
}

MultiMongo::MultiMongo(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context),
      multi_mongo_(
          config.Name(),
          context.FindComponent<Secdist>().GetStorage(),
          ParsePoolConfig(config),
          clients::dns::GetResolverPtr(config, context),
          context.FindComponent<DynamicConfig>().GetSource()
      )
{
    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();
    statistics_holder_ =
        statistics_storage.GetStorage()
            .RegisterWriter(multi_mongo_.GetName(), [this](utils::statistics::Writer& writer) {
                writer = multi_mongo_;
            });
}

MultiMongo::~MultiMongo() { statistics_holder_.Unregister(); }

storages::mongo::PoolPtr MultiMongo::GetPool(const std::string& dbalias) const { return multi_mongo_.GetPool(dbalias); }

void MultiMongo::AddPool(std::string dbalias) { multi_mongo_.AddPool(std::move(dbalias)); }

bool MultiMongo::RemovePool(const std::string& dbalias) { return multi_mongo_.RemovePool(dbalias); }

storages::mongo::MultiMongo::PoolSet MultiMongo::NewPoolSet() { return multi_mongo_.NewPoolSet(); }

yaml_config::Schema MultiMongo::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/mongo/component_multi.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
