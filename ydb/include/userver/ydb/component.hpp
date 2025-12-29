#pragma once

/// @file userver/ydb/component.hpp
/// @brief @copybrief ydb::YdbComponent

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/formats/json.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/ydb/fwd.hpp>

#include <ydb-cpp-sdk/client/driver/fwd.h>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {
class Driver;
}  // namespace impl

/// @ingroup userver_components
///
/// @brief YDB client component
///
/// Provides access to @ref ydb::TableClient, @ref ydb::TopicClient, @ref ydb::FederatedTopicClient,
/// @ref ydb::CoordinationClient.
///
/// ## Static options of ydb::YdbComponent :
/// @include{doc} scripts/docs/en/components_schema/ydb/src/ydb/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class YdbComponent final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ydb::YdbComponent component
    static constexpr std::string_view kName = "ydb";

    YdbComponent(const components::ComponentConfig&, const components::ComponentContext&);

    ~YdbComponent() override;

    /// Get table client
    /// @param dbname database name from static config key
    std::shared_ptr<TableClient> GetTableClient(const std::string& dbname) const;

    /// Get topic client
    /// @param dbname database name from static config key
    std::shared_ptr<TopicClient> GetTopicClient(const std::string& dbname) const;

    /// Get topic client
    /// @param dbname database name from static config key
    std::shared_ptr<FederatedTopicClient> GetFederatedTopicClient(const std::string& dbname) const;

    /// Get coordination client
    /// @param dbname database name from static config key
    std::shared_ptr<CoordinationClient> GetCoordinationClient(const std::string& dbname) const;

    /// Get native driver
    /// @param dbname database name from static config key
    /// @warning Use with care! Facilities from
    /// `<core/include/userver/drivers/subscribable_futures.hpp>` can help with
    /// non-blocking wait operations.
    const NYdb::TDriver& GetNativeDriver(const std::string& dbname) const;

    /// Get database path
    /// @param dbname database name from static config key
    const std::string& GetDatabasePath(const std::string& dbname) const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    struct DatabaseUtils;

    struct Database {
        std::shared_ptr<impl::Driver> driver;
        std::shared_ptr<TableClient> table_client;
        std::shared_ptr<TopicClient> topic_client;
        std::shared_ptr<FederatedTopicClient> federated_topic_client;
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
