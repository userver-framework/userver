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

class ClickHouse : public LoggableComponentBase {
 public:
  ClickHouse(const ComponentConfig&, const ComponentContext&);
  ~ClickHouse() override;

  std::shared_ptr<storages::clickhouse::Cluster> GetCluster() const;

 private:
  clients::dns::Component& dns_;

  std::shared_ptr<storages::clickhouse::Cluster> cluster_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace components

USERVER_NAMESPACE_END
