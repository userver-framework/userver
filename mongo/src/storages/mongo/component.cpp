#include <userver/storages/mongo/component.hpp>

#include <boost/algorithm/string/predicate.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/manager.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool_config.hpp>

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

enum class ResolverType {
  kGetaddrinfo,
  kAsync,
};

ResolverType ParseDnsResolver(const ComponentConfig& config) {
  auto str = config["dns_resolver"].As<std::string>("getaddrinfo");
  if (str == "getaddrinfo") return ResolverType::kGetaddrinfo;
  if (str == "async") return ResolverType::kAsync;

  throw storages::mongo::InvalidConfigException()
      << "Invalid value '" << str << "' for dns_resolver";
}

clients::dns::Resolver* GetResolverPtr(const ComponentConfig& config,
                                       const ComponentContext& context) {
  if (ParseDnsResolver(config) == ResolverType::kGetaddrinfo) {
    return nullptr;
  } else {
    return &context.FindComponent<clients::dns::Component>().GetResolver();
  }
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

  auto* dns_resolver = GetResolverPtr(config, context);

  storages::mongo::PoolConfig pool_config(config);

  pool_ = std::make_shared<storages::mongo::Pool>(
      config.Name(), connection_string, pool_config, dns_resolver);

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
}

Mongo::~Mongo() { statistics_holder_.Unregister(); }

storages::mongo::PoolPtr Mongo::GetPool() const { return pool_; }

MultiMongo::MultiMongo(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase(config, context),
      multi_mongo_(config.Name(), context.FindComponent<Secdist>().GetStorage(),
                   storages::mongo::PoolConfig(config),
                   GetResolverPtr(config, context)),
      is_verbose_stats_enabled_(ParseStatsVerbosity(config)) {
  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      multi_mongo_.GetName(), [this](const auto&) { return GetStatistics(); });
}

MultiMongo::~MultiMongo() { statistics_holder_.Unregister(); }

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

}  // namespace components

USERVER_NAMESPACE_END
