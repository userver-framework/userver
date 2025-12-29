#pragma once

/// @file userver/storages/clickhouse/component.hpp
/// @brief @copybrief components::ClickHouse

#include <userver/components/component_base.hpp>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace storages::clickhouse {
class Cluster;
}

namespace components {

/// @ingroup userver_components
///
/// @brief ClickHouse client component
///
/// Provides access to a ClickHouse cluster.
///
/// ## Static configuration example:
///
/// @snippet samples/clickhouse_service/static_config.yaml  Clickhouse service sample - static config
///
/// If the component is configured with an secdist_alias, it will lookup
/// connection data in secdist.json via secdist_alias value, otherwise via components name.
///
/// ## Secdist format
///
/// A ClickHouse alias in secdist is described as a JSON object
/// `clickhouse_settings`, containing descriptions of databases.
///
/// @snippet samples/clickhouse_service/tests/conftest.py  Clickhouse service sample - secdist
///
/// ## Static options of components::ClickHouse :
/// @include{doc} scripts/docs/en/components_schema/clickhouse/src/storages/clickhouse/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class ClickHouse : public ComponentBase {
public:
    /// Component constructor
    ClickHouse(const ComponentConfig&, const ComponentContext&);
    /// Component destructor
    ~ClickHouse() override;

    /// Cluster accessor
    std::shared_ptr<storages::clickhouse::Cluster> GetCluster() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    clients::dns::Component& dns_;

    std::shared_ptr<storages::clickhouse::Cluster> cluster_;
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<ClickHouse> = true;

}  // namespace components

USERVER_NAMESPACE_END
