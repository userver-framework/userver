#include "config.hpp"

#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ydb/impl/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

template <typename T>
T MergeWithSecdist(const std::optional<T>& secdist_field,
                   std::optional<T>&& config_field,
                   const yaml_config::YamlConfig& dbconfig,
                   std::string_view field_name) {
  if (secdist_field) {
    return *secdist_field;
  }
  if (config_field) {
    return *std::move(config_field);
  }
  throw yaml_config::Exception(
      fmt::format("Missing required field '{}.{}' with no override in secdist",
                  dbconfig.GetPath(), field_name));
}

}  // namespace

TableSettings ParseTableSettings(const yaml_config::YamlConfig& dbconfig,
                                 const secdist::DatabaseSettings& dbsecdist) {
  TableSettings result;

  result.min_pool_size =
      dbconfig["min_pool_size"].As<std::uint32_t>(result.min_pool_size);
  result.max_pool_size =
      dbconfig["max_pool_size"].As<std::uint32_t>(result.max_pool_size);
  result.keep_in_query_cache =
      dbconfig["keep-in-query-cache"].As<bool>(result.keep_in_query_cache);

  result.sync_start = dbconfig["sync_start"].As<bool>(result.sync_start);

  result.by_database_timings_buckets =
      dbconfig["by-database-timings-buckets-ms"]
          .As<std::optional<std::vector<double>>>();
  result.by_query_timings_buckets =
      dbconfig["by-query-timings-buckets-ms"]
          .As<std::optional<std::vector<double>>>();

  result.sync_start = dbsecdist.sync_start.value_or(result.sync_start);

  return result;
}

DriverSettings ParseDriverSettings(
    const yaml_config::YamlConfig& dbconfig,
    const secdist::DatabaseSettings& dbsecdist,
    std::shared_ptr<NYdb::ICredentialsProviderFactory>
        credentials_provider_factory) {
  DriverSettings result;

  auto config_endpoint = dbconfig["endpoint"].As<std::optional<std::string>>();
  auto config_database = dbconfig["database"].As<std::optional<std::string>>();

  result.prefer_local_dc =
      dbconfig["prefer_local_dc"].As<bool>(result.prefer_local_dc);

  result.endpoint = MergeWithSecdist(
      dbsecdist.endpoint, std::move(config_endpoint), dbconfig, "endpoint");
  result.database = MergeWithSecdist(
      dbsecdist.database, std::move(config_database), dbconfig, "database");
  result.oauth_token = dbsecdist.oauth_token;
  if (dbsecdist.iam_jwt_params.has_value()) {
    result.iam_jwt_params =
        formats::json::ToString(dbsecdist.iam_jwt_params.value());
  }

  result.credentials_provider_factory = std::move(credentials_provider_factory);

  return result;
}

using OptMs = std::optional<std::chrono::milliseconds>;

ConfigCommandControl Parse(const formats::json::Value& config,
                           formats::parse::To<ConfigCommandControl>) {
  return ConfigCommandControl{
      config["attempts"].As<std::optional<int>>(),
      config["operation-timeout-ms"].As<OptMs>(),
      config["cancel-after-ms"].As<OptMs>(),
      config["client-timeout-ms"].As<OptMs>(),
      config["get-session-timeout-ms"].As<OptMs>(),
  };
}

const dynamic_config::Key<std::unordered_map<std::string, ConfigCommandControl>>
    kQueryCommandControl("YDB_QUERIES_COMMAND_CONTROL",
                         dynamic_config::DefaultAsJsonString("{}"));

const dynamic_config::Key<int> kDeadlinePropagationVersion(
    "YDB_DEADLINE_PROPAGATION_VERSION", 1);

const dynamic_config::Key<
    std::unordered_map<std::string, utils::RetryBudgetSettings>>
    kRetryBudgetSettings("YDB_RETRY_BUDGET",
                         dynamic_config::DefaultAsJsonString("{}"));

}  // namespace ydb::impl

USERVER_NAMESPACE_END
