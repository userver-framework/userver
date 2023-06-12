#pragma once

/// @file userver/storages/mysql/component.hpp
/// @brief @copybrief storages::mysql::Component

#include <memory>

#include <userver/components/loggable_component_base.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace storages::mysql {
class Cluster;
}

namespace storages::mysql {

// clang-format off
/// @ingroup userver_components
///
/// @brief MySQL/MariaDB client component
///
/// Provides access to a MySQL/MariaDB cluster.
///
/// ## Static configuration example:
///
/// @snippet samples/mysql_service/static_config.yaml  MySQL service sample - static config
///
/// The component will lookup connection data in secdist.json via its name.
///
/// ## Secdist format:
///
/// Connection settings are described as a JSON object `mysql_settings`,
/// containing descriptions of MySQL/MariaDB clusters.
///
/// @snippet samples/mysql_service/tests/conftest.py  Mysql service sample - secdist
///
/// **Important note**: the uMySQL driver does **NOT** perform an automatic
/// primary detection and always considers the first host in `hosts` list
/// an only primary node in the cluster.
///
/// ## Static options (more to come)
/// Name                     | Description                                 | Default value
/// -------------------------|---------------------------------------------|---------------
/// initial_pool_size        | initial connection pool size (per host)     | 5
/// max_pool_size            | maximum connection pool size (per host)     | 10
///
// clang-format on
class Component final : public components::LoggableComponentBase {
 public:
  /// Component constructor
  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  /// Component destructor
  ~Component() override;

  /// Cluster accessor
  std::shared_ptr<storages::mysql::Cluster> GetCluster() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  clients::dns::Component& dns_;

  const std::shared_ptr<storages::mysql::Cluster> cluster_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
