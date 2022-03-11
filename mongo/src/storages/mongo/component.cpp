#include <userver/storages/mongo/component.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/manager.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool_config.hpp>

#include <storages/mongo/mongo_config.hpp>
#include <storages/mongo/mongo_secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

const std::string kStandardMongoPrefix = "mongo-";

bool ParseStatsVerbosity(const ComponentConfig& config) {
  const auto verbosity_str = config["stats_verbosity"].As<std::string>("terse");
  if (verbosity_str == "terse") return false;
  if (verbosity_str == "full") return true;

  throw storages::mongo::InvalidConfigException()
      << "Invalid value '" << verbosity_str << "' for stats_verbosity";
}

storages::mongo::Config GetInitConfig(const ComponentContext& context) {
  auto config_source = context.FindComponent<TaxiConfig>().GetSource();
  auto snapshot = config_source.GetSnapshot();
  return snapshot.Get<storages::mongo::Config>();
}

}  // namespace

Mongo::Mongo(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase(config, context),
      is_verbose_stats_enabled_(ParseStatsVerbosity(config)) {
  auto dbalias = config["dbalias"].As<std::string>("");

  std::string connection_string;
  if (!dbalias.empty()) {
    connection_string = storages::mongo::secdist::GetSecdistConnectionString(
        context.FindComponent<Secdist>().GetStorage(), dbalias);
  } else {
    connection_string = config["dbconnection"].As<std::string>();
  }

  auto* dns_resolver = clients::dns::GetResolverPtr(config, context);

  storages::mongo::PoolConfig pool_config(config);
  auto config_source = context.FindComponent<TaxiConfig>().GetSource();
  auto snapshot = config_source.GetSnapshot();

  pool_ = std::make_shared<storages::mongo::Pool>(
      config.Name(), connection_string, pool_config, dns_resolver,
      snapshot.Get<storages::mongo::Config>());

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();

  auto section_name = config.Name();
  if (boost::algorithm::starts_with(section_name, kStandardMongoPrefix) &&
      section_name.size() != kStandardMongoPrefix.size()) {
    section_name = section_name.substr(kStandardMongoPrefix.size());
  }
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      "mongo." + section_name, [this](const auto&) {
        return is_verbose_stats_enabled_ ? pool_->GetVerboseStatistics()
                                         : pool_->GetStatistics();
      });

  config_subscription_ = config_source.UpdateAndListen(
      this, "mongo-config-updater", &Mongo::OnConfigUpdate);
}

Mongo::~Mongo() {
  config_subscription_.Unsubscribe();
  statistics_holder_.Unregister();
}

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

void Mongo::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
  pool_->SetConfig(cfg.Get<storages::mongo::Config>());
}

std::string Mongo::GetStaticConfigSchema() {
  return R"(
type: object
description: mongo config
additionalProperties: false
properties:
    dbalias:
        type: string
        description: name of the database in secdist config (if available)
    dbconnection:
        type: string
        description: connection string (used if no dbalias specified)
    appname:
        type: string
        description: application name for the DB server 
        defaultDescription: userver
    conn_timeout:
        type: string
        description: connection timeout 
        defaultDescription: 2s
    so_timeout:
        type: string
        description: socket timeout 
        defaultDescription: 10s
    queue_timeout:
        type: string
        description: max connection queue wait time 
        defaultDescription: 1s
    initial_size:
        type: integer
        description: number of connections created initially 
        defaultDescription: 16
    max_size:
        type: integer
        description: limit for total connections number 
        defaultDescription: 128
    idle_limit:
        type: integer
        description: limit for idle connections number 
        defaultDescription: 64
    connecting_limit:
        type: integer
        description: limit for establishing connections number 
        defaultDescription: 8
    local_threshold:
        type: string
        description: latency window for instance selection 
        defaultDescription: mongodb default
    max_replication_lag:
        type: string
        description: replication lag limit for usable secondaries, min. 90s
    maintenance_period:
        type: string
        description: pool maintenance period (idle connections pruning etc.) 
        defaultDescription: 15s
    stats_verbosity:
        type: string
        description: changes the granularity of reported metrics 
        defaultDescription: 'terse'
    dns_resolver:
        type: string
        description: server hostname resolver type (getaddrinfo or async)
        defaultDescription: 'getaddrinfo'
)";
}

MultiMongo::MultiMongo(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      multi_mongo_(config.Name(), context.FindComponent<Secdist>().GetStorage(),
                   storages::mongo::PoolConfig(config),
                   clients::dns::GetResolverPtr(config, context),
                   GetInitConfig(context)),
      is_verbose_stats_enabled_(ParseStatsVerbosity(config)) {
  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      multi_mongo_.GetName(), [this](const auto&) { return GetStatistics(); });

  auto config_source = context.FindComponent<TaxiConfig>().GetSource();
  config_subscription_ = config_source.UpdateAndListen(
      this, "multi-mongo-config-updater", &MultiMongo::OnConfigUpdate);
}

MultiMongo::~MultiMongo() {
  config_subscription_.Unsubscribe();
  statistics_holder_.Unregister();
}

storages::mongo::PoolPtr MultiMongo::GetPool(const std::string& dbalias) const {
  return multi_mongo_.GetPool(dbalias);
}

void MultiMongo::AddPool(std::string dbalias) {
  multi_mongo_.AddPool(std::move(dbalias));
}

bool MultiMongo::RemovePool(const std::string& dbalias) {
  return multi_mongo_.RemovePool(dbalias);
}

storages::mongo::MultiMongo::PoolSet MultiMongo::NewPoolSet() {
  return multi_mongo_.NewPoolSet();
}

formats::json::Value MultiMongo::GetStatistics() const {
  return multi_mongo_.GetStatistics(is_verbose_stats_enabled_);
}

void MultiMongo::OnConfigUpdate(const dynamic_config::Snapshot& cfg) {
  multi_mongo_.SetConfig(cfg.Get<storages::mongo::Config>());
}

std::string MultiMongo::GetStaticConfigSchema() {
  return R"(
type: object
description: multi-mongo config
additionalProperties: false
properties:
    appname:
        type: string
        description: application name for the DB server
        defaultDescription: userver
    conn_timeout:
        type: string
        description: connection timeout
        defaultDescription: 2s
    so_timeout:
        type: string
        description: socket timeout
        defaultDescription: 10s
    queue_timeout:
        type: string
        description: max connection queue wait time
        defaultDescription: 1s
    initial_size:
        type: string
        description: number of connections created initially (per database)
        defaultDescription: 16
    max_size:
        type: integer
        description: limit for total connections number (per database)
        defaultDescription: 128
    idle_limit:
        type: integer
        description: limit for idle connections number (per database)
        defaultDescription: 64
    connecting_limit:
        type: integer
        description: limit for establishing connections number (per database)
        defaultDescription: 8
    local_threshold:
        type: string
        description: latency window for instance selection
        defaultDescription: mongodb default
    max_replication_lag:
        type: string
        description: replication lag limit for usable secondaries, min. 90s
    stats_verbosity:
        type: string
        description: changes the granularity of reported metrics
        defaultDescription: 'terse'
    dns_resolver:
        type: string
        description: server hostname resolver type (getaddrinfo or async)
        defaultDescription: 'getaddrinfo'
)";
}

}  // namespace components

USERVER_NAMESPACE_END
