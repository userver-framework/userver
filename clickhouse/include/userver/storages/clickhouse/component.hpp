#pragma once

/// @file userver/storages/clickhouse/component.hpp
/// @brief @copybrief components::ClickHouse

#include <userver/components/loggable_component_base.hpp>

#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace storages::clickhouse {
class Cluster;
}

namespace components {

// clang-format off

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
/// connection data in secdist.json via secdist_alias value, otherwise via
/// components name.
///
/// ## Secdist format
///
/// A ClickHouse alias in secdist is described as a JSON object
/// `clickhouse_settings`, containing descriptions of databases.
///
/// @snippet samples/clickhouse_service/tests/conftest.py  Clickhouse service sample - secdist
///
/// ## Static options:
/// Name                  | Description                                      | Default value
/// --------------------- | ------------------------------------------------ | ---------------
/// secdist_alias         | name of the key in secdist config                | components name
/// initial_pool_size     | number of connections created initially          | 5
/// max_pool_size         | maximum number of created connections            | 10
/// queue_timeout         | client waiting for a free connection time limit  | 1s
/// use_secure_connection | whether to use TLS for connections               | true
/// compression           | compression method to use (none / lz4)           | none

// clang-format on

class ClickHouse : public LoggableComponentBase {
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
