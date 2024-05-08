#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <ydb-cpp-sdk/client/types/credentials/credentials.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ydb/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {
struct RetryBudgetSettings;
}  // namespace utils

namespace ydb::impl {

namespace secdist {
struct DatabaseSettings;
}  // namespace secdist

struct TableSettings {
  std::uint32_t min_pool_size{10};
  std::uint32_t max_pool_size{50};
  bool keep_in_query_cache{true};
  bool sync_start{true};
  std::optional<std::vector<double>> by_database_timings_buckets{};
  std::optional<std::vector<double>> by_query_timings_buckets{};
};

struct TopicSettings {};

struct DriverSettings {
  std::string endpoint;
  std::string database;

  bool prefer_local_dc{false};
  std::optional<std::string> oauth_token;
  std::optional<std::string> iam_jwt_params;
  std::shared_ptr<NYdb::ICredentialsProviderFactory>
      credentials_provider_factory;
};

TableSettings ParseTableSettings(const yaml_config::YamlConfig& dbconfig,
                                 const secdist::DatabaseSettings& dbsecdist);

DriverSettings ParseDriverSettings(
    const yaml_config::YamlConfig& dbconfig,
    const secdist::DatabaseSettings& dbsecdist,
    std::shared_ptr<NYdb::ICredentialsProviderFactory>
        credentials_provider_factory);

struct ConfigCommandControl {
  std::optional<std::uint32_t> attempts;
  std::optional<std::chrono::milliseconds> operation_timeout_ms;
  std::optional<std::chrono::milliseconds> cancel_after_ms;
  std::optional<std::chrono::milliseconds> client_timeout_ms;
  std::optional<std::chrono::milliseconds> get_session_timeout_ms;
};

ConfigCommandControl Parse(const formats::json::Value& config,
                           formats::parse::To<ConfigCommandControl>);

extern const dynamic_config::Key<
    std::unordered_map<std::string, ConfigCommandControl>>
    kQueryCommandControl;

inline constexpr int kDeadlinePropagationExperimentVersion = 1;

extern const dynamic_config::Key<int> kDeadlinePropagationVersion;

extern const dynamic_config::Key<
    std::unordered_map<std::string, utils::RetryBudgetSettings>>
    kRetryBudgetSettings;

}  // namespace ydb::impl

USERVER_NAMESPACE_END
