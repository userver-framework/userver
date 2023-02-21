#pragma once

#include <memory>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
class Component;
}

namespace storages::mysql {
class Cluster;
}

namespace components {

class MySQL final : public LoggableComponentBase {
 public:
  MySQL(const ComponentConfig& config, const ComponentContext& context);

  ~MySQL() override;

  std::shared_ptr<storages::mysql::Cluster> GetCluster() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  clients::dns::Component& dns_;

  std::shared_ptr<storages::mysql::Cluster> cluster_;
};

}  // namespace components

USERVER_NAMESPACE_END
