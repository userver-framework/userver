#pragma once

/// @file userver/storages/odbc/component.hpp
/// @brief @copybrief components::Odbc

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/secdist/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {
class Cluster;
}

namespace components {

/// @ingroup userver_components
///
/// @brief ODBC client component that owns a storages::odbc::Cluster.
///
/// ## Dynamic options:
/// * @ref USERVER_ODBC_DEFAULT_COMMAND_CONTROL
/// * @ref USERVER_ODBC_HANDLERS_COMMAND_CONTROL
/// * @ref USERVER_ODBC_QUERIES_COMMAND_CONTROL
/// * @ref USERVER_ODBC_CONNECTION_POOL_SETTINGS
/// * @ref USERVER_ODBC_STATEMENT_METRICS_SETTINGS
/// * @ref USERVER_ODBC_PREPARED_STATEMENT_CACHE_SETTINGS
///
/// ## Static configuration example:
///
/// @snippet odbc/functional_tests/basic_chaos/static_config.yaml ODBC component config
///
/// Exactly one of `dsn`, `pools`, and `secdist_alias` must be specified.
/// With `secdist_alias`, connection data is loaded from components::Secdist and
/// is updated without changing the Cluster object returned by GetCluster().
///
/// ## Static options of components::Odbc:
/// @include{doc} scripts/docs/en/components_schema/odbc/src/storages/odbc/component.md
///
/// Options inherited from @ref components::ComponentBase:
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class Odbc final : public ComponentBase {
public:
    static constexpr std::string_view kName = "odbc";

    Odbc(const ComponentConfig& config, const ComponentContext& context);
    ~Odbc() override;

    std::shared_ptr<storages::odbc::Cluster> GetCluster() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& config);
    void OnSecdistUpdate(const storages::secdist::SecdistConfig& secdist);

    std::string name_;
    storages::odbc::settings::StatementMetricsSettings statement_metrics_settings_fallback_;
    std::optional<std::string> secdist_alias_;
    std::shared_ptr<storages::odbc::Cluster> cluster_;

    dynamic_config::Source config_source_;
    concurrent::AsyncEventSubscriberScope config_subscription_;
    concurrent::AsyncEventSubscriberScope secdist_subscription_;
};

template <>
inline constexpr bool kHasValidate<Odbc> = true;

}  // namespace components

USERVER_NAMESPACE_END
