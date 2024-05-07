#pragma once

/// @file userver/ydb/component.hpp
/// @brief @copybrief ydb::YdbComponent

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/formats/json.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/ydb/fwd.hpp>

namespace NYdb {
class TDriver;
}

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
}  // namespace impl

// clang-format off

/// @ingroup userver_components
///
/// @brief YDB client component
///
/// Provides access to ydb::TableClient.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// credentials-provider | name of credentials provider component | -
/// operation-settings.retries | default retries count for an operation | 3
/// operation-settings.operation-timeout | default operation timeout in utils::StringToDuration() format | 1s
/// operation-settings.cancel-after | cancel operation after specified string in utils::StringToDuration() format | 1s
/// operation-settings.client-timeout | default client timeout in utils::StringToDuration() format | 1s
/// operation-settings.get-session-timeout | default session timeout in milliseconds | 5s
/// databases.<dbname>.endpoint | gRPC endpoint URL, e.g. grpc://localhost:1234 | -
/// databases.<dbname>.database | full database path, e.g. /ru/service/production/database | -
/// databases.<dbname>.credentials | credentials config passed to credentials provider component | -
/// databases.<dbname>.min_pool_size | minimum pool size for database with name <dbname> | 10
/// databases.<dbname>.max_pool_size | maximum pool size for database with name <dbname> | 50
/// databases.<dbname>.keep-in-query-cache | whether to use query cache | true
/// databases.<dbname>.prefer_local_dc | prefer making requests to local DataCenter | false
/// databases.<dbname>.aliases | list of alias names for this database | []
/// databases.<dbname>.sync_start | fail on boot time if YDB is not accessible | true
/// databases.<dbname>.by-database-timings-buckets-ms | histogram bounds for by-database timing metrics | 40 buckets with +20% increment per step
/// databases.<dbname>.by-query-timings-buckets-ms | histogram bounds for by-query timing metrics | 15 buckets with +100% increment per step

// clang-format on

class YdbComponent final : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of ydb::YdbComponent component
  static constexpr std::string_view kName = "ydb";

  YdbComponent(const components::ComponentConfig&,
               const components::ComponentContext&);

  ~YdbComponent();

  std::shared_ptr<TableClient> GetTableClient(const std::string& dbname) const;

  std::shared_ptr<TopicClient> GetTopicClient(const std::string& dbname) const;

  std::shared_ptr<CoordinationClient> GetCoordinationClient(
      const std::string& dbname) const;

  const NYdb::TDriver& GetNativeDriver(const std::string& dbname) const;

  const std::string& GetDatabasePath(const std::string& dbname) const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  struct DatabaseUtils;

  struct Database {
    std::shared_ptr<impl::Driver> driver;
    std::shared_ptr<TableClient> table_client;
    std::shared_ptr<TopicClient> topic_client;
    std::shared_ptr<CoordinationClient> coordination_client;
  };

  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);
  void WriteStatistics(utils::statistics::Writer& writer) const;
  const Database& FindDatabase(const std::string& dbname) const;

  std::unordered_map<std::string, Database> databases_;

  dynamic_config::Source config_;

  // These fields must be the last ones
  concurrent::AsyncEventSubscriberScope config_subscription_;
  utils::statistics::Entry statistic_holder_;
};

}  // namespace ydb

template <>
inline constexpr bool components::kHasValidate<ydb::YdbComponent> = true;

USERVER_NAMESPACE_END
